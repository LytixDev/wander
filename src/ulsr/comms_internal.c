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
#include "ulsr/comms_external.h"
#include "ulsr/comms_internal.h"
#include "ulsr/impl.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/routing.h"


static void packet_use_route(struct ulsr_internal_packet *packet, struct node_t *node)
{
    struct route_t *new_route = (struct route_t *)queue_pop(node->route_queue);
    u16 new_len = new_route->path_length + packet->route->step;
    packet->route->path = realloc(packet->route->path, new_len * sizeof(u16));

    for (int i = packet->route->step; i < packet->route->step + new_route->path_length; i++) {
	packet->route->path[i] = new_route->path[i - packet->route->step];
    }

    packet->route->len = new_len;
    packet->route->step++;

    node->send_func(packet, packet->route->path[packet->route->step]);
}

static void packet_bogo_and_find_route(struct ulsr_internal_packet *packet, struct node_t *node)
{
    packet->route->len++;
    packet->route->step++;
    packet->route->path = realloc(packet->route->path, packet->route->len * sizeof(u16));
    packet->route->path[packet->route->step] = find_random_neighbor(node);
    node->send_func(packet, packet->route->path[packet->route->step]);

    /* This is called because this node doesn't have any routes to the destination */

    find_all_routes(node, MESH_NODE_COUNT);
}

static void handle_internal_data_packet(struct node_t *node, struct ulsr_internal_packet *packet)
{
    struct ulsr_packet *payload = packet->payload;
    LOG_NODE_INFO(node->node_id, "Received packet from rec_func");
    LOG_NODE_INFO(node->node_id, "Received packet");
    LOG_NODE_INFO(node->node_id, "Source: %s", payload->source_ipv4);
    LOG_NODE_INFO(node->node_id, "Destination: %s", payload->dest_ipv4);
    // LOG_NODE_INFO(node->node_id, "Payload: %s", payload->payload);

    // Checks if node is at final hop in the packet's route
    if (node->node_id == packet->route->path[packet->route->len - 1]) {
	// Checks if packet can be sent to external network
	if (node->can_connect_func(node) && !packet->is_response) {
	    LOG_NODE_INFO(node->node_id, "Node can connect to external network");
	    handle_send_external(node, packet);
	    LOG_NODE_INFO(node->node_id, "Node sent packet to external network");
	} else if (packet->is_response) {
	    handle_send_external(node, packet);
	    LOG_NODE_INFO(node->node_id, "Node sent packet to receiver");
	} else {
	    packet->prev_node_id = node->node_id;
	    LOG_NODE_INFO(node->node_id, "Node cannot connect to external network");
	    // Check if packet can be sent to another node that can connect to external network
	    if (!queue_empty(node->route_queue)) {
		packet_use_route(packet, node);
	    } else {
		// Send packet to random neighbor
		packet_bogo_and_find_route(packet, node);
	    }
	}
    } else {
	LOG_NODE_INFO(node->node_id, "Packet is not for this node");
	packet->route->step++;
	packet->prev_node_id = node->node_id;
	node->send_func(packet, packet->route->path[packet->route->step]);
    }

    // free(packet);
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

static void handle_internal_routing_packet(struct node_t *node, struct ulsr_internal_packet *packet)
{
    LOG_NODE_INFO(node->node_id, "Received ROUTING from %d", packet->prev_node_id);
    struct routing_data_t *routing_data = packet->payload;
    find_all_routes_send(node, routing_data->total_nodes, routing_data->visited, routing_data->path,
			 routing_data->path_length, routing_data->time_taken);
}

static void handle_internal_routing_packet_done(struct node_t *node,
						struct ulsr_internal_packet *packet)
{
    LOG_NODE_INFO(node->node_id, "Received ROUTING_DONE from %d", packet->prev_node_id);
    struct route_payload_t *route = (struct route_payload_t *)packet->payload;
    if (packet->dest_node_id == node->node_id) {
	queue_push(node->route_queue, route->route);
	LOG_NODE_INFO(node->node_id, "Found route to %d",
		      route->route->path[route->route->path_length - 1]);
    } else {
	packet->prev_node_id = node->node_id;
	node->send_func(
	    packet,
	    route->route->path[route->route->path_length - ++route->step_from_destination - 1]);
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


u16 find_random_neighbor(struct node_t *node)
{
    struct neighbor_t *neighbors[MESH_NODE_COUNT];
    u16 counter = 0;
    for (u16 i = 0; i < MESH_NODE_COUNT; i++) {
	if (node->neighbors[i] != 0)
	    neighbors[counter++] = node->neighbors[i];
    }

    if (counter == 0)
	return 0;

    return neighbors[rand() % counter]->node_id;
}


void handle_send_internal(void *arg)
{
    struct node_t *node = (struct node_t *)arg;
    struct ulsr_internal_packet *packet = NULL;

    while (node->running) {
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
	    handle_internal_routing_packet(node, packet);
	    break;

	case PACKET_ROUTING_DONE:
	    handle_internal_routing_packet_done(node, packet);
	    break;
	default:
	    break;
	}
	free(packet);
    }
}
