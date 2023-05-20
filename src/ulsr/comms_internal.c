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

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/logger.h"
#include "lib/queue.h"
#include "ulsr/comms_external.h"
#include "ulsr/comms_internal.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/routing.h"


void propogate_failure(struct ulsr_internal_packet *packet, struct node_t *node)
{
    struct ulsr_packet *failure_packet = ulsr_create_failure(packet->payload);
    struct ulsr_internal_packet *failure_internal = ulsr_internal_from_external(failure_packet);
    failure_internal->pr = reverse_packet_route(packet->pr);
    failure_internal->is_response = true;
    failure_internal->prev_node_id = node->node_id;
    LOG_ERR("PACKET COULD NOT BE ROUTED :-(");
    use_packet_route(failure_internal, node);
}

static u16 bogo_find_neighbor_stub(struct node_t *node, struct packet_route_t *pt, u16 *ignore_list,
				   u16 *ignore_len)
{
    u16 next_hop_id = find_random_neighbor(node, pt->path, pt->step, ignore_list, *ignore_len);
    ignore_list[(*ignore_len)++] = next_hop_id;
    pt->path[pt->step + 1] = next_hop_id;
    return next_hop_id;
}

bool send_bogo(struct ulsr_internal_packet *packet, struct node_t *node)
{
    // LOG_NODE_INFO(node->node_id, "use bogo, step %d", packet->pr->step);

    packet->pr->path = realloc(packet->pr->path, (packet->pr->step + 2) * sizeof(u16));
    packet->pr->len = packet->pr->step + 1;

    /* find random neighbor until packet sent or no more neighbors */
    u16 ignore_list[node->known_nodes_count];
    u16 ignore_len = 0;
    bool came_through;
    u16 next_hop_id = bogo_find_neighbor_stub(node, packet->pr, ignore_list, &ignore_len);

    while (next_hop_id != 0) {
	packet->pr->len++;
	came_through = use_packet_route(packet, node);
	if (came_through) {
	    /* This is called because this node doesn't have any routes to the destination */
	    find_all_routes(node, node->known_nodes_count);
	    return true;
	}
	/* was not routed, so len needs to be decremented before call to find neighbor */
	packet->pr->len--;
	next_hop_id = bogo_find_neighbor_stub(node, packet->pr, ignore_list, &ignore_len);
    }

    /* packet got stuck in bogo :-( */
    propogate_failure(packet, node);
    /* This is called because this node doesn't have any routes to the destination */
    find_all_routes(node, node->known_nodes_count);
    return false;
}

bool use_packet_route(struct ulsr_internal_packet *packet, struct node_t *node)
{
    LOG_NODE_INFO(node->node_id, "Use packet route");
    packet->pr->step++;
    packet->prev_node_id = node->node_id;
    bool came_through = node->send_func(packet, packet->pr->path[packet->pr->step]) != -1;
    if (!came_through)
	packet->pr->step--;
    return came_through;
}

u16 find_random_neighbor(struct node_t *node, u16 *path, u16 path_len, u16 *ignore_list,
			 u16 ignore_len)
{
    pthread_mutex_lock(&node->neighbor_list_lock);

    struct neighbor_t *neighbors[node->known_nodes_count];
    u16 counter = 0;
    for (u16 i = 0; i < node->known_nodes_count; i++) {
	/* if node is in ignore list, ignore */
	if (ignore_list != NULL) {
	    for (u16 j = 0; j < ignore_len; j++) {
		if (ignore_list[j] == i + 1) {
		    goto ignore;
		}
	    }
	}

	/* if neighbor */
	if (node->neighbors[i] != NULL) {
	    /* if already used in path, ignore */
	    for (u16 j = 0; j < path_len; j++) {
		if (path[j] == i + 1)
		    goto ignore;
	    }
	    neighbors[counter++] = node->neighbors[i];
	}
ignore:
	continue;
    }

    pthread_mutex_unlock(&node->neighbor_list_lock);

    if (counter == 0)
	return 0;

    return neighbors[rand() % counter]->node_id;
}

static void handle_data_packet(struct node_t *node, struct ulsr_internal_packet *packet)
{
    LOG_NODE_INFO(node->node_id, "Received data packet from %d", packet->prev_node_id);
    /* check last node in route is this node */
    if (node->node_id == packet_route_final_hop(packet->pr)) {
	// TODO: here we assume that the final hop of a route that is a response can connect to the
	// client
	/* check if data is for client */
	if (packet->is_response) {
	    handle_send_external(node, packet);
	    return;
	}

	/* packet is for external network, so check if node can connect to external network */
	if (node->can_connect_func(node)) {
	    LOG_NODE_INFO(node->node_id, "Sending to external");
	    // TODO: handle failure
	    handle_send_external(node, packet);
	    return;
	}

	/*
	 * Strategy when packet can't be sent to external, and we have no further path.
	 * 1. Try to use an existing route on the node
	 * 2. If no existing route on node: bogo
	 */

	/* 1 */
	if (!queue_empty(node->route_queue)) {
	    struct packet_route_t *append = route_to_packet_route(queue_pop(node->route_queue));
	    struct packet_route_t *pt = packet_route_combine(packet->pr, append);

	    //     Check if we can free the path here
	    //     free(packet->pt->path);
	    //     free(packet->pt);

	    packet->pr = pt;
	    bool came_through = use_packet_route(packet, node);
	    if (came_through)
		return;

	    /* path failed */
	    came_through = send_bogo(packet, node);
	    if (!came_through)
		propogate_failure(packet, node);

	    /* 2 */
	} else {
	    /* send packet to random neighbor */
	    bool came_through = send_bogo(packet, node);
	    if (!came_through)
		propogate_failure(packet, node);
	}

    } else {
	/* use next hop from route */
	bool came_through = use_packet_route(packet, node);
	if (came_through)
	    return;

	/* */

	/* path failed */
	came_through = send_bogo(packet, node);
	if (!came_through)
	    propogate_failure(packet, node);
    }
}

static void handle_hello_packet(struct node_t *node, struct ulsr_internal_packet *packet)
{
    u16 neighbor_id = packet->prev_node_id;

    pthread_mutex_lock(&node->neighbor_list_lock);
    struct neighbor_t *neighbor = node->neighbors[neighbor_id - 1];
    if (neighbor == NULL) {
	LOG_NODE_INFO(node->node_id, "Found new neighbor %d", neighbor_id);
	neighbor = malloc(sizeof(struct neighbor_t));
	neighbor->node_id = neighbor_id;
	node->neighbors[neighbor_id - 1] = neighbor;
    }
    neighbor->last_seen = time(NULL);

    pthread_mutex_unlock(&node->neighbor_list_lock);
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

#ifdef LOG_ALL_INTERNAL_INCOMING
	LOG_NODE_INFO(node->node_id, "Received packet type %s from %d",
		      uslr_internal_type_to_str[packet->type], packet->prev_node_id);
#endif

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
	for (size_t i = 0; i < ARRAY_LEN(node->known_nodes); i++) {
	    u16 to_id = ARRAY_GET(node->known_nodes, i);
	    if (to_id == node->node_id)
		continue;

	    struct ulsr_internal_packet *packet = ulsr_internal_create_hello(node->node_id, to_id);
	    node->send_func(packet, to_id);
	    free(packet);
	}

	/* check if any neighbors are "out of date" */
	remove_old_neighbors(node);

	sleep(node->hello_poll_interval);
    }
}
