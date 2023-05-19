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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/routing.h"

void init_routing_data(u16 source_id, char destination_ipv4[16], u16 total_nodes, bool *visited, u16 *path,
		       u16 path_length, u32 time_taken, struct routing_data_t *routing_data)
{
    routing_data->source_id = source_id;
    routing_data->destination_ipv4 = strdup(destination_ipv4);
    routing_data->total_nodes = total_nodes;

    routing_data->visited = malloc(sizeof(bool) * total_nodes);
    memcpy(routing_data->visited, visited, sizeof(bool) * total_nodes);

    routing_data->path = malloc(sizeof(u16) * total_nodes);
    memcpy(routing_data->path, path, sizeof(u16) * total_nodes);

    routing_data->path_length = path_length;

    routing_data->time_taken = time_taken;
}

void init_route(u16 source_id, char destination_ipv4[16], u16 *path, u16 path_length, u32 time_taken,
		struct route_t *route)
{
    route->source_id = source_id;
    route->destination_ipv4 = strdup(destination_ipv4);
    route->path = malloc(sizeof(u16) * path_length);
    memcpy(route->path, path, sizeof(u16) * path_length);
    route->path_length = path_length;

    route->time_taken = time_taken;
}

void free_routing_data(struct routing_data_t *routing_data)
{
    free(routing_data->visited);
    free(routing_data->path);
}

void free_route(struct route_t *route)
{
    free(route->path);
}

void find_all_routes_send(struct node_t *curr, char destination_ipv4[16], u16 total_nodes, bool *visited,
			  u16 *path, u16 path_length, u32 time_taken)
{
    visited[curr->node_id - 1] = true;
    path[path_length++] = curr->node_id;

    if (curr->range_func(destination_id)) {
	struct route_t *route = malloc(sizeof(struct route_t));
	init_route(curr->node_id, destination_id, path, path_length, time_taken, route);
	struct route_payload_t *route_payload = malloc(sizeof(struct route_payload_t));
	route_payload->route = route;
	route_payload->step_from_destination = 0;
	struct ulsr_internal_packet *packet = malloc(sizeof(struct ulsr_internal_packet));
	packet->type = PACKET_ROUTING_DONE;
	packet->payload = route;
	packet->payload_len = sizeof(struct route_t) + sizeof(u16) * path_length;
	packet->prev_node_id = curr->node_id;
	packet->dest_node_id = path[0];
	packet->checksum = 0;
	curr->send_func(packet, path[path_length - route_payload->step_from_destination - 1]);
    } else {
	struct neighbor_t *neighbor = NULL;
	for (u16 i = 0; i < total_nodes; i++) {
	    neighbor = curr->neighbors[i];
	    if (neighbor == NULL || visited[neighbor->node_id - 1]) {
		continue;
	    }

	    struct routing_data_t *routing_data = malloc(sizeof(struct routing_data_t));
	    init_routing_data(curr->node_id, destination_id, total_nodes, visited, path,
			      path_length, time_taken, routing_data);
	    struct ulsr_internal_packet *packet = malloc(sizeof(struct ulsr_internal_packet));
	    packet->type = PACKET_ROUTING;
	    packet->payload = routing_data;
	    packet->payload_len = sizeof(struct routing_data_t) + sizeof(bool) * total_nodes +
				  sizeof(u16) * total_nodes;
	    packet->prev_node_id = curr->node_id;
	    packet->dest_node_id = neighbor->node_id;
	    packet->checksum = 0;
	    curr->send_func(packet, neighbor->node_id);
	}
    }
}

void find_all_routes(struct node_t *start, char *destination_ipv4, u16 total_nodes)
{
    bool visited[total_nodes];
    memset(visited, false, total_nodes);
    u16 path[total_nodes];
    u16 path_length = 0;
    u32 time_taken = 0;

    find_all_routes_send(start, destination_ipv4, total_nodes, visited, path, path_length,
			 time_taken);
}

u32 find_longest_time(struct route_array_t *routes)
{
    struct route_t *route = NULL;
    u16 i = 0;
    u32 longest_time = 0;
    ARRAY_FOR_EACH(routes, i, route)
    {
	if (route->time_taken > longest_time) {
	    longest_time = route->time_taken;
	}
    }

    return longest_time;
}

struct route_t *get_random_route(struct route_array_t *routes)
{
    return ARRAY_GET(routes, (u16)rand() % routes->len);
}

u16 *reverse_route(u16 *route, u16 route_length)
{
    u16 *reversed_route = malloc(sizeof(u16) * route_length);
    u16 i = 0;
    for (i = 0; i < route_length; i++) {
	reversed_route[i] = route[route_length - i - 1];
    }
    for (i = 0; i < route_length; i++) {
	printf("%d ", reversed_route[i]);
    }
    return reversed_route;
}