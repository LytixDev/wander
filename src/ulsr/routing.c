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
#include "ulsr/routing.h"

void find_all_routes_recursive(struct node_t *source, struct node_t *destination, u16 total_nodes,
			       bool *visited, u16 *path, u64 path_length,
			       struct u16_arraylist_t *routes)
{
    visited[source->node_id - 1] = true;
    path[path_length++] = source->node_id;

    if (source->node_id == destination->node_id) {
	u16 *new_path = malloc(sizeof(u16) * path_length);
	memcpy(new_path, path, sizeof(u16) * path_length);
	ARRAY_PUSH(*routes, new_path);
    } else {
	u16 i = 0;
	struct neighbor_t *neighbor = NULL;
	ARRAY_FOR_EACH(*source->neighbors, i, neighbor)
	{
	    if (!visited[neighbor->node_id - 1]) {
                // Send the packet with the new path to the neighbor
		find_all_routes_recursive(neighbor, destination, total_nodes, visited, path,
					  path_length, routes);
	    }
	}
    }

    visited[source->node_id - 1] = false;
    path_length--;
}

void find_all_routes(struct node_t *start, struct node_t *destination, u16 total_nodes,
		     struct u16_arraylist_t *routes)
{
    bool visited[total_nodes];
    memset(visited, false, total_nodes);
    u16 path[total_nodes];
    u64 path_length = 0;

    find_all_routes_recursive(start, destination, total_nodes, visited, path, path_length, routes);
}