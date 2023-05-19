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
#include "lib/logger.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/routing.h"

// TODO: so many memory leaks....
void init_routing_data(u16 source_id, u16 total_nodes, bool *visited, u16 *path, u16 path_length,
		       u32 time_taken, struct routing_data_t *routing_data)
{
    routing_data->source_id = source_id;

    routing_data->total_nodes = total_nodes;

    routing_data->visited = malloc(sizeof(bool) * total_nodes);
    memcpy(routing_data->visited, visited, sizeof(bool) * total_nodes);

    routing_data->path = malloc(sizeof(u16) * total_nodes);
    memcpy(routing_data->path, path, sizeof(u16) * total_nodes);

    routing_data->path_length = path_length;

    routing_data->time_taken = time_taken;
}

void init_route(u16 source_id, u16 destination_id, u16 *path, u16 path_length, u32 time_taken,
		struct route_t *route)
{
    route->source_id = source_id;
    route->destination_id = destination_id;
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

void find_all_routes_send(struct node_t *curr, u16 total_nodes, bool *visited, u16 *path,
			  u16 path_length, u32 time_taken)
{
    visited[curr->node_id - 1] = true;
    path[path_length++] = curr->node_id;

    if (curr->can_connect_func(curr)) {
	struct route_t *route = malloc(sizeof(struct route_t));
	init_route(path[0], curr->node_id, path, path_length, time_taken, route);
	struct route_payload_t *route_payload = malloc(sizeof(struct route_payload_t));
	route_payload->route = route;
	route_payload->step_from_destination = 1;
	struct ulsr_internal_packet *packet = malloc(sizeof(struct ulsr_internal_packet));
	packet->type = PACKET_ROUTING_DONE;
	packet->payload = route_payload;
	packet->payload_len =
	    sizeof(struct route_t) + sizeof(u16) * path_length + sizeof(struct route_payload_t);
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
	    init_routing_data(curr->node_id, total_nodes, visited, path, path_length, time_taken,
			      routing_data);
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

void find_all_routes(struct node_t *start, u16 total_nodes)
{
    LOG_INFO("Finding all routes from node %d", start->node_id);
    bool visited[total_nodes];
    memset(visited, false, total_nodes);
    u16 path[total_nodes];
    u16 path_length = 0;
    u32 time_taken = 0;

    find_all_routes_send(start, total_nodes, visited, path, path_length, time_taken);
}

u16 *reverse_route(u16 *path, u16 route_length)
{
    u16 *reversed_route = malloc(sizeof(u16) * route_length);
    u16 i = 0;
    for (i = 0; i < route_length; i++) {
	reversed_route[i] = path[route_length - i - 1];
    }

    return reversed_route;
}

struct packet_route_t *reverse_packet_route(struct packet_route_t *pt)
{
    struct packet_route_t *reversed = malloc(sizeof(struct route_t));
    u16 *path = reverse_route(pt->path, pt->step); // TODO: step correct?
    reversed->path = path;
    reversed->len = pt->step;
    reversed->step = 0;
    return reversed;
}
