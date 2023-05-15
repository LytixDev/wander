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
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/threadpool.h"

/**
 * Arraylist struct for sockaddr_in.
 */
ARRAY_T(sockaddr_in_array_t, struct sockaddr_in)

/**
 * Arraylist struct for neighbor_t.
 */
ARRAY_T(neighbor_array_t, struct neighbor_t)

/**
 * Struct used to represent a neighbor of a node.
 * @param addr The address of the neighbor.
 * @param cost The cost to the neighbor.
 */
struct neighbor_t {
    struct sockaddr_in addr;
    u16 cost;
};

/**
 * Struct for connections.
 * @param connections The connections.
 * @param index The current index of the connections.
 * @param cap The max amount of connections.
 */
struct connections_t {
    int *connections;
    int index;
    int cap;
};

/**
 * Struct used to represent a node in the network.
 * @param socket The socket of the node.
 * @param running Whether the node is running.
 * @param connections The connections of the node.
 * @param threadpool The threadpool of the node.
 * @param neighbors The neighbors of the node.
 * @param all_nodes All nodes in the network.
 */
struct node_t {
    int socket;
    bool running;
    struct connections_t *connections;
    struct threadpool_t *threadpool;
    struct neighbor_array_t *neighbors;
    struct sockaddr_in_array_t *all_nodes;
};

/* Methods */

/**
 * Initializes a node.
 * @param node The node to initialize.
 * @param connections The amount of connections.
 * @param threads The amount of threads.
 * @param queue_size The size of the queue.
 * @param ... The current nodes known in the network.
 */
int init_node(struct node_t *node, int connections, int threads, int queue_size, ...);

/**
 * Frees a node.
 * @param node The node to free.
 */
void free_node(struct node_t *node);

#endif /* NODE_H */
