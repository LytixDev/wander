/*
 *  Copyright (C) 2023 Nicolai Brand, Callum Gran
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "lib/common.h"
#include "lib/logger.h"
#include "ulsr/comms_external.h"
#include "ulsr/comms_internal.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/routing.h"
#include "ulsr/ulsr.h"


bool handle_send_external(struct node_t *node, struct ulsr_internal_packet *packet)
{
    struct ulsr_packet *internal_payload = packet->payload;
    LOG_NODE_INFO(node->node_id, "Handling outgoing request to IP %s at port %d",
		  internal_payload->dest_ipv4, internal_payload->dest_port);

    int ext_sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (ext_sockfd < 0) {
	LOG_NODE_ERR(node->node_id, "Failed to create socket");
	return false;
    }

    struct sockaddr_in server = { 0 };
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(internal_payload->dest_ipv4);
    server.sin_port = htons(internal_payload->dest_port);

    if (connect(ext_sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
	LOG_NODE_ERR(node->node_id, "Failed to connect to %s/%d", internal_payload->dest_ipv4,
		     internal_payload->dest_port);
	return false;
    }

    /* send packet to external entity */
    if (send(ext_sockfd, internal_payload->payload, internal_payload->payload_len, 0) < 0) {
	LOG_NODE_ERR(node->node_id, "Failed to send packet to %s/%d", internal_payload->dest_ipv4,
		     internal_payload->dest_port);
	return false;
    }

    if (!packet->is_response) {
	u8 response[UINT16_MAX];
	memset(response, 0, UINT16_MAX);
	/* reverse the route we used to come here, and use that for the response */
	u16 *reversed = reverse_route(packet->pr->path, packet->pr->len);
	u32 seq_nr = 0;

	while (node->running && recv(ext_sockfd, response, UINT16_MAX - 1, 0) > 0) {
	    struct ulsr_packet *ret_packet =
		ulsr_create_response(internal_payload, response, seq_nr);
	    struct ulsr_internal_packet *internal_packet = ulsr_internal_from_external(ret_packet);

	    /* init route */
	    internal_packet->pr = malloc(sizeof(struct packet_route_t));
	    internal_packet->pr->len = packet->pr->len;
	    internal_packet->pr->step = 1;
	    internal_packet->pr->path = reversed;
	    internal_packet->prev_node_id = node->node_id;
	    internal_packet->dest_node_id = packet->pr->path[0];
	    internal_packet->is_response = true;

	    // LOG_NODE_INFO(node->node_id, "Route length: %d", internal_packet->route->len);
	    // for (int i = 0; i < internal_packet->route->len; i++) {
	    //     LOG_NODE_INFO(node->node_id, "Route: %d", internal_packet->route->path[i]);
	    // }
	    node->send_func(internal_packet, internal_packet->pr->path[internal_packet->pr->step]);
	    // LOG_NODE_INFO(node->node_id, "Sent return packet with seq_nr %d to node %d", seq_nr,
	    //		  internal_packet->route->path[internal_packet->route->step]);
	    seq_nr++;
	}
    }

    close(ext_sockfd);
    return true;
}

void handle_external(void *arg)
{
    struct external_request_thread_data_t *data = (struct external_request_thread_data_t *)arg;
    struct node_t *node = data->node;
    struct ulsr_packet packet = { 0 };

    ssize_t bytes_read = recv(data->connection, &packet, sizeof(struct ulsr_packet), 0);
    if (bytes_read <= 0) {
	LOG_NODE_ERR(node->node_id, "ABORT!: Failed to read from socket");
	goto cleanup;
    }

    LOG_NODE_INFO(node->node_id, "Received external packet");

    /* validate checksum */
    // u32 checksum = ulsr_checksum((u8 *)&packet, (unsigned long)bytes_read);
    // if (checksum != packet.checksum) {
    //     LOG_NODE_ERR(node->node_id, "ABORT!: Checksum failed!");
    //     goto cleanup;
    // }

    /* pack external packet into internal packet for routing between nodes */
    struct ulsr_internal_packet *internal_packet = ulsr_internal_from_external(&packet);
    internal_packet->prev_node_id = node->node_id;

    /* find path to destination */
    if (!queue_empty(node->route_queue)) {
	struct packet_route_t *pr = route_to_packet_route(queue_pop(node->route_queue));
	internal_packet->pr = pr;
	bool came_through = use_packet_route(internal_packet, node);
	if (came_through)
	    return;

	/* path failed */
	came_through = send_bogo(internal_packet, node);
	if (!came_through)
	    propogate_failure(internal_packet, node);
    } else {
	/* should be a function */
	internal_packet->pr = malloc(sizeof(struct packet_route_t));
	internal_packet->pr->len = 1;
	internal_packet->pr->step = 0;
	internal_packet->pr->path = malloc(sizeof(u16));
	internal_packet->pr->path[0] = node->node_id;

	/* find random neighbor to forward the packet to */
	bool came_trough = send_bogo(internal_packet, node);
	if (!came_trough) {
	    propogate_failure(internal_packet, node);
	}

	find_all_routes(data->node, node->known_nodes_count);
    }

cleanup:
    shutdown(data->connection, SHUT_RDWR);
    close(data->connection);
    free(data);
}
