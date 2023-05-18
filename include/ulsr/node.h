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
#include "ulsr/packet.h"

/* Function definitions. */

/**
 * Function definition for a function that finds the distance to a node.
 */
typedef u16 (*node_distance_func_t)(void *);

/**
 * Function definition for a function that sends a message.
 */
typedef u16 (*node_send_func_t)(struct ulsr_internal_packet *packet, u16 node_id);

/**
 * Function definition for a function that receives a message.
 */
typedef struct ulsr_internal_packet *(*node_recv_func_t)(u16 node_id);

/**
 * Function definition for a function that frees the data of a node.
 */
typedef void (*data_free_func_t)(void *);

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
    u16 node_id;
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
 * @param node_id The id of the node.
 * @param sockfd The socket file descriptor.
 * @param running Whether the node is running.
 * @param data The data of the node.
 * @param distance_func The function used to find the distance to a node.
 * @param send_func The function used to send a message.
 * @param rec_func The function used to receive a message.
 * @param connections The connections of the node.
 * @param threadpool The threadpool of the node.
 * @param neighbors The neighbors of the node.
 */
struct node_t {
    u16 node_id;
    int sockfd;
    bool running;
    void *data;
    data_free_func_t data_free_func;
    node_distance_func_t distance_func;
    node_send_func_t send_func;
    node_recv_func_t rec_func;
    struct connections_t *connections;
    struct threadpool_t *threadpool;
    struct neighbor_array_t *neighbors;
};


/* Methods */

/**
 * Initializes a node.
 * @param node The node to initialize.
 * @param node_id The id of the node.
 * @param connections The amount of connections.
 * @param threads The amount of threads.
 * @param queue_size The size of the queue.
 * @param ... The current nodes known in the network.
 */
bool init_node(struct node_t *node, u16 node_id, u16 connections, u16 threads, u16 queue_size,
	       node_distance_func_t distance_func, node_send_func_t send_func,
	       node_recv_func_t rec_func, void *data, data_free_func_t data_free_func, u16 port);

/**
 * Runs a node.
 * @param node The node to run.
 */
int run_node(struct node_t *node);

/**
 * Frees a node.
 * @param node The node to free.
 */
void free_node(struct node_t *node);

void close_node(struct node_t *node);

#endif /* NODE_H */
