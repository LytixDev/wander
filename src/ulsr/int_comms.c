
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

#include <stdlib.h>
#include <string.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/logger.h"
#include "ulsr/ext_comms.h"
#include "ulsr/impl.h"
#include "ulsr/int_comms.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/route_table.h"
#include "ulsr/routing.h"


static void handle_internal_data_packet(struct node_t *node, struct ulsr_internal_packet *packet)
{
    struct ulsr_packet *payload = packet->payload;
    LOG_NODE_INFO(node->node_id, "Received packet from rec_func");
    LOG_NODE_INFO(node->node_id, "Received packet");
    LOG_NODE_INFO(node->node_id, "Source: %s", payload->source_ipv4);
    LOG_NODE_INFO(node->node_id, "Destination: %s", payload->dest_ipv4);
    // LOG_NODE_INFO(node->node_id, "Payload: %s", payload->payload);

    if (node->node_id == packet->dest_node_id) {
	LOG_NODE_INFO(node->node_id, "Packet is for this node");
	handle_send_external(node, packet);
    } else {
	LOG_NODE_INFO(node->node_id, "Packet is not for this node");
	packet->route->step++;
	packet->prev_node_id = node->node_id;
	node->send_func(packet, packet->route->path[packet->route->step]);
    }
}

static void handle_internal_hello_packet(struct node_t *node, struct ulsr_internal_packet *packet)
{
    // LOG_NODE_INFO(node->node_id, "Received HELLO from %d", packet->prev_node_id);
    u16 neighbor_id = packet->prev_node_id;
    struct neighbor_t *neighbor = node->neighbors[neighbor_id - 1];
    if (neighbor == NULL) {
	LOG_NODE_INFO(node->node_id, "Found new neighbor %d", neighbor_id);
	neighbor = malloc(sizeof(struct neighbor_t));
	neighbor->node_id = neighbor_id;
	node->neighbors[neighbor_id - 1] = neighbor;
    }
    neighbor->last_seen = time(NULL);
}

void handle_send_internal(void *arg)
{
    struct node_t *node = (struct node_t *)arg;
    struct ulsr_internal_packet *packet = NULL;

    while (node->running) {
	/* allocates the packet for us */
	packet = node->rec_func(node->node_id);
	if (packet == NULL) {
	    continue;
	}

	switch (packet->type) {
	case PACKET_DATA:
	    handle_internal_data_packet(node, packet);
	    break;

	case PACKET_HELLO:
	    handle_internal_hello_packet(node, packet);
	    break;

	case PACKET_PURGE:
	    LOG_NODE_INFO(node->node_id, "Received PURGE packet");
	    break;

	case PACKET_ROUTING:
	    LOG_NODE_INFO(node->node_id, "Received ROUTING packet");
	    break;

	case PACKET_ROUTING_DONE:
	    LOG_NODE_INFO(node->node_id, "Received ROUTING_DONE packet");
	    break;
	default:
	    break;
	}

	free(packet);
	packet = NULL;
    }
}


void hello_poll_thread(void *arg)
{
    struct node_t *node = (struct node_t *)arg;
    while (node->running) {
	for (size_t i = 0; i < ARRAY_LEN(node->known_ids); i++) {
	    u16 to_id = ARRAY_GET(node->known_ids, i);
	    if (to_id == node->node_id)
		continue;

	    // LOG_NODE_INFO(node->node_id, "Sent HELLO to %d", to_id);

	    struct ulsr_internal_packet *packet = ulsr_internal_create_hello(node->node_id, to_id);

	    node->send_func(packet, to_id);
	    free(packet);
	}
	sleep(HELLO_POLL_INTERVAL);
    }
}
