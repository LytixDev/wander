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
    struct route_t *route_to_use = (struct route_t *)queue_pop(node->route_queue);
    u16 new_len = route_to_use->path_length + packet->route->step;
    packet->route->path = realloc(packet->route->path, new_len * sizeof(u16));

    for (int i = packet->route->step; i < packet->route->step + route_to_use->path_length; i++) {
	packet->route->path[i] = route_to_use->path[i - packet->route->step];
    }

    /* has been copied over and served its cause */
    free(route_to_use);

    packet->route->len = new_len;
    packet->route->step++;
    node->send_func(packet, packet->route->path[packet->route->step]);
}

static void packet_bogo_and_find_route(struct ulsr_internal_packet *packet, struct node_t *node)
{
    packet->route->len++;
    packet->route->step++;
    packet->route->path = realloc(packet->route->path, packet->route->len * sizeof(u16));
    // TODO: handle edge cases where find random "fails"
    packet->route->path[packet->route->step] = find_random_neighbor(node);
    node->send_func(packet, packet->route->path[packet->route->step]);

    /* This is called because this node doesn't have any routes to the destination */
    find_all_routes(node, MESH_NODE_COUNT);
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

static void handle_data_packet(struct node_t *node, struct ulsr_internal_packet *packet)
{
    /* check if route destination is this node */
    if (node->node_id == packet->route->path[packet->route->len - 1]) {
	/* checks if packet can be sent to external network */
	if (node->can_connect_func(node)) {
	    handle_send_external(node, packet);
	    return;
	}

	packet->prev_node_id = node->node_id;
	/* check if packet can be sent to another node that can connect to external network */
	if (!queue_empty(node->route_queue)) {
	    packet_use_route(packet, node);
	} else {
	    /* send packet to random neighbor */
	    // TODO: handle edge case where 1. no neighbors, and 2. all neighbors have been "used
	    // up"
	    packet_bogo_and_find_route(packet, node);
	}

    } else {
	/* use next hop from route */
	packet->route->step++;
	packet->prev_node_id = node->node_id;
	node->send_func(packet, packet->route->path[packet->route->step]);
    }
}

static void handle_hello_packet(struct node_t *node, struct ulsr_internal_packet *packet)
{
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

static void handle_routing_packet(struct node_t *node, struct ulsr_internal_packet *packet)
{
    struct routing_data_t *routing_data = packet->payload;
    find_all_routes_send(node, routing_data->total_nodes, routing_data->visited, routing_data->path,
			 routing_data->path_length, routing_data->time_taken);
}

static void handle_routing_done_packet(struct node_t *node, struct ulsr_internal_packet *packet)
{
    struct route_payload_t *route = (struct route_payload_t *)packet->payload;
    if (packet->dest_node_id == node->node_id) {
	queue_push(node->route_queue, route->route);
	// LOG_NODE_INFO(node->node_id, "Found route to %d",
	//	      route->route->path[route->route->path_length - 1]);
    } else {
	packet->prev_node_id = node->node_id;
	node->send_func(
	    packet,
	    route->route->path[route->route->path_length - ++route->step_from_destination - 1]);
    }
}

void main_recv_thread(void *arg)
{
    struct node_t *node = (struct node_t *)arg;
    struct ulsr_internal_packet *packet = NULL;

    while (node->running) {
	packet = node->recv_func(node->node_id);
	if (packet == NULL) {
	    continue;
	}

	LOG_NODE_INFO(node->node_id, "Received packet type %s from %d",
		      uslr_internal_type_to_str[packet->type], packet->prev_node_id);

	switch (packet->type) {
	case PACKET_DATA:
	    handle_data_packet(node, packet);
	    break;

	case PACKET_HELLO:
	    handle_hello_packet(node, packet);
	    break;

	case PACKET_PURGE:
	    LOG_NODE_INFO(node->node_id, "Received PURGE packet");
	    break;

	case PACKET_ROUTING:
	    handle_routing_packet(node, packet);
	    break;

	case PACKET_ROUTING_DONE:
	    handle_routing_done_packet(node, packet);
	    break;
	default:
	    LOG_NODE_ERR(node->node_id, "Received unknown packet that wasn't parsed");
	    break;
	}

	free(packet);
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

	    struct ulsr_internal_packet *packet = ulsr_internal_create_hello(node->node_id, to_id);
	    node->send_func(packet, to_id);
	    free(packet);
	}

	/* check if any neighbors are "out of date" */
	// TODO: is a new mutex needed, or can I reuse the node lock ?
	remove_old_neighbors(node);
	sleep(HELLO_POLL_INTERVAL);
    }
}
