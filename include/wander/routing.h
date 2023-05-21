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

#ifndef ROUTING_H
#define ROUTING_H

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "wander/node.h"

#define NS_TO_US(ns) ((ns) / 1000)
#define SEC_TO_NS(sec) ((sec)*1000000000)
#define MAX_ROUTE_TIME 10000000

struct packet_route_t {
    u16 *path;
    u16 len;
    u16 step;
    bool has_slept;
};

/**
 * Struct for data to be sent to the routing thread
 */
struct routing_data_t {
    u16 source_id;
    u16 total_nodes;
    bool *visited;
    u16 *path;
    u16 path_length;
    u64 time_taken;
};

/**
 * Struct for a calculated route
 */
struct route_t {
    u16 source_id;
    u16 destination_id;
    u16 *path;
    u16 path_length;
    u64 time_taken;
};

/**
 * Struct for a route payload
 */
struct route_payload_t {
    struct route_t *route;
    u16 step_from_destination;
};

/* Methods */

/**
 * Initializes the routing data struct
 *
 * @param source_id The source node ID
 * @param destination_id The destination node ID
 * @param total_nodes The total number of nodes in the network
 * @param visited The visited array
 * @param path The path array
 * @param path_length The length of the path
 * @param time_taken The time taken to reach the current node
 * @param routing_data The routing data struct to initialize
 */
void init_routing_data(u16 source_id, u16 total_nodes, bool *visited, u16 *path, u16 path_length,
		       u64 time_taken, struct routing_data_t *routing_data);

/**
 * Initializes the route struct
 *
 * @param source_id The source node ID
 * @param destination_id The destination node ID
 * @param path The path array
 * @param path_length The length of the path
 * @param time_taken The time taken to reach the current node
 * @param route The route struct to initialize
 */
void init_route(u16 source_id, u16 destination_id, u16 *path, u16 path_length, u64 time_taken,
		struct route_t *route);

/**
 * Frees the routing data struct
 *
 * @param routing_data The routing data struct to free
 */
void free_routing_data(struct routing_data_t *routing_data);

/**
 * Frees the route struct
 *
 * @param route The route struct to free
 */
void free_route(struct route_t *route);

/**
 * Method that is called by each node to find all routes to the destination node
 * This function then sends the routes to the next node in the path
 *
 * @param curr The current node
 * @param destination_id The destination node ID
 * @param total_nodes The total number of nodes in the network
 * @param visited The visited array
 * @param path The path array
 * @param path_length The length of the path
 * @param time_taken The time taken to reach the current node
 */
void find_all_routes_send(struct node_t *curr, u16 total_nodes, bool *visited, u16 *path,
			  u16 path_length, u64 time_taken);

/**
 * Finds all routes from the current node to the destination node
 *
 * @param start The starting node
 * @param destination_id The destination node ID
 * @param total_nodes The total number of nodes in the network
 */
void find_all_routes(struct node_t *start, u16 total_nodes);

u16 *reverse_route(u16 *route, u16 route_length);

struct packet_route_t *reverse_packet_route_from_step(struct packet_route_t *pt);

u16 packet_route_next_hop(struct packet_route_t *pt);

u16 packet_route_final_hop(struct packet_route_t *pt);

void route_sleep(struct route_t *route);

struct packet_route_t *packet_route_combine(struct packet_route_t *a, struct packet_route_t *b);

struct packet_route_t *route_to_packet_route(struct route_t *route);

void route_free(struct route_t *route);

void packet_route_free(struct packet_route_t *pr);

#endif /* ROUTING_H */
