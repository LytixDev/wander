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

#include <stdbool.h>
#include <stdlib.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/queue.h"
#include "lib/threadpool.h"
#include "ulsr/packet.h"

/* Function definitions. */

/**
 * Function definition for a function that sends a message.
 */
typedef u16 (*node_send_func_t)(struct ulsr_internal_packet *packet, u16 node_id);

/**
 * Function definition for a function that receives a message.
 */
typedef struct ulsr_internal_packet *(*node_recv_func_t)(u16 node_id);

/* opaque bro */
struct node_t;

/**
 * Function definition for a function that checks if a given mesh node is connected to the internet
 * of the target
 */
typedef bool (*node_can_connect_func_t)(struct node_t *node);

/**
 * Function definition for a function that frees the data of a node.
 */
typedef void (*data_free_func_t)(void *);

ARRAY_T(u16_arraylist_t, u16);

/**
 * Struct used to represent a neighbor of a node.
 * @param node_id The id of the neighbor.
 */
struct neighbor_t {
    u16 node_id;
    time_t last_seen;
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
    node_can_connect_func_t can_connect_func;
    node_send_func_t send_func;
    node_recv_func_t recv_func;
    struct connections_t *connections;
    struct threadpool_t *threadpool;
    struct queue_t *route_queue;
    struct u16_arraylist_t *known_ids;
    struct neighbor_t **neighbors;
};


/* Methods */

/**
 * Initializes a node.
 * @param node The node to initialize.
 * @param node_id The id of the node.
 * @param connections The amount of connections.
 * @param threads The amount of threads.
 * @param queue_size The size of the queue.
 */
bool init_node(struct node_t *node, u16 node_id, u16 connections, u16 threads, u16 queue_size,
	       node_can_connect_func_t can_connect_func, node_send_func_t send_func,
	       node_recv_func_t rec_func, u16 port);

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

void remove_old_neighbors(struct node_t *node);

#endif /* NODE_H */
