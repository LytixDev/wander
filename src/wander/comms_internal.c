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
#include <time.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/logger.h"
#include "lib/queue.h"
#include "wander/comms_external.h"
#include "wander/comms_internal.h"
#include "wander/node.h"
#include "wander/packet.h"
#include "wander/routing.h"


void propagate_failure(struct wander_internal_packet *packet, struct node_t *node)
{
    LOG_NODE_ERR(node->node_id, "PACKET COULD NOT BE ROUTED: PROPOGATE FAILURE");

    struct wander_packet *failure_packet = wander_create_failure(packet->payload);
    struct wander_internal_packet *failure_internal = wander_internal_from_external(failure_packet);

    /* create reversed route */
    u16 failure_step = packet->pr->step;
    failure_internal->pr = reverse_packet_route_from_step(packet->pr);
    /* can fail at any step of the route, but so we have to start the reversed route from where we
     * failed */
    failure_internal->pr->step = failure_internal->pr->len - failure_step;
    failure_internal->pr->step = 0;
    failure_internal->prev_node_id = node->node_id;
    failure_internal->is_response = true;

    bool came_through;
    /* edge case where reversed len is 1 */
    if (failure_internal->pr->len == 1) {
	came_through = handle_send_external(node, failure_internal);
	if (!came_through) {
	    LOG_NODE_ERR(node->node_id, "PROPOGATE FAILURE ... FAILED");
	}
	return;
    }

    came_through = use_packet_route(failure_internal, node);
    if (!came_through) {
	LOG_NODE_ERR(node->node_id, "PROPOGATE FAILURE ... FAILED");
    }
}

static u16 bogo_find_neighbor_stub(struct node_t *node, struct packet_route_t *pt, u16 *ignore_list,
				   u16 *ignore_len)
{
    u16 next_hop_id = find_random_neighbor(node, pt->path, pt->step, ignore_list, *ignore_len);
    ignore_list[(*ignore_len)++] = next_hop_id;
    pt->path[pt->step + 1] = next_hop_id;
    return next_hop_id;
}

bool send_bogo(struct wander_internal_packet *packet, struct node_t *node)
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
    find_all_routes(node, node->known_nodes_count);
    return false;
}

bool use_packet_route(struct wander_internal_packet *packet, struct node_t *node)
{
    // LOG_NODE_INFO(node->node_id, "Use packet route");
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

static void handle_data_packet(struct node_t *node, struct wander_internal_packet *packet)
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
	    if (!packet->pr->has_slept)
		packet->pr->has_slept = true;

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
	if (!route_table_empty(node->routing_table)) {
	    struct route_t *route = get_random_route(node->routing_table);

	    if (!packet->pr->has_slept) {
		packet->pr->has_slept = true;
		route_sleep(route);
	    }
	    struct packet_route_t *append = route_to_packet_route(route);
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
		propagate_failure(packet, node);

	    /* 2 */
	} else {
	    /* send packet to random neighbor */
	    bool came_through = send_bogo(packet, node);
	    if (!came_through)
		propagate_failure(packet, node);
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
	    propagate_failure(packet, node);
    }
}

static void handle_hello_packet(struct node_t *node, struct wander_internal_packet *packet)
{
    u16 neighbor_id = packet->prev_node_id;

    pthread_mutex_lock(&node->neighbor_list_lock);
    struct neighbor_t *neighbor = node->neighbors[neighbor_id - 1];
    if (neighbor == NULL) {
	node->new_neighbors_count++;
	LOG_NODE_INFO(node->node_id, "Found new neighbor %d", neighbor_id);
	neighbor = malloc(sizeof(struct neighbor_t));
	neighbor->node_id = neighbor_id;
	node->neighbors[neighbor_id - 1] = neighbor;
    }
    neighbor->last_seen = time(NULL);

    pthread_mutex_unlock(&node->neighbor_list_lock);
}

static void handle_routing_packet(struct node_t *node, struct wander_internal_packet *packet)
{
    struct routing_data_t *routing_data = packet->payload;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    u64 nanosecs = SEC_TO_NS((uint64_t)ts.tv_sec) + (uint64_t)ts.tv_nsec;
    routing_data->time_taken = NS_TO_US(nanosecs) - routing_data->time_taken;
    find_all_routes_send(node, routing_data->total_nodes, routing_data->visited, routing_data->path,
			 routing_data->path_length, routing_data->time_taken);
}

static void handle_routing_done_packet(struct node_t *node, struct wander_internal_packet *packet)
{
    struct route_payload_t *route = (struct route_payload_t *)packet->payload;
    if (packet->dest_node_id == node->node_id) {
	if (route->route->time_taken > MAX_ROUTE_TIME) {
	    // TODO: free route or something
	    return;
	}
	add_last_pos(node->routing_table, route->route);
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
    struct wander_internal_packet *packet = NULL;

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

	    struct wander_internal_packet *packet =
		wander_internal_create_hello(node->node_id, to_id);
	    node->send_func(packet, to_id);
	    free(packet);
	}

	/* check if any neighbors are "out of date" */
	remove_old_neighbors(node);

	sleep(node->hello_poll_interval);
    }
}
