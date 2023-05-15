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

#ifndef NODE_H
#define NODE_H

#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

/**
 * Struct used to represent a neighbor of a node.
 * @param addr The address of the neighbor.
 * @param cost The cost to the neighbor.
 */
struct neighbor_t {
    struct sockaddr_in addr;
    int cost;
};

/**
 * Struct used to represent a node in the network.
 * @param addr The address of the node.
 * @param neighbors The neighbors of the node.
 * @param all_nodes All nodes in the network.
 */
struct node_t {
    struct sockaddr_in addr;
    struct neighbor_t *neighbors;
    struct sockaddr_in *all_nodes;
};

#endif /* NODE_H */