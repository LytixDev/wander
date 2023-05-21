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

#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/logger.h"
#include "lib/queue.h"
#include "wander/comms_external.h"
#include "wander/comms_internal.h"
#include "wander/node.h"
#include "wander/routing_table.h"


static void close_all_external_connections(struct connections_t *connections)
{
    for (int i = 0; i < connections->cap; i++) {
	if (send(connections->connections[i], "q", 2, MSG_NOSIGNAL) > 0) {
	    shutdown(connections->connections[i], SHUT_RDWR);
	    close(connections->connections[i]);
	}
    }
}

static void insert_external_connection(struct connections_t *connections, int connection)
{
    connections->index++;
    connections->index = connections->index % connections->cap;
    connections->connections[connections->index] = connection;
}

void remove_route_with_old_neighbor(struct node_t *node, u16 invalid_node_id)
{
    /* if route table already empty, do nothing */
    if (route_table_empty(node->routing_table))
	return;

    struct route_iter_t iter;
    iter_start(&iter, node->routing_table);
    while (!iter_end(&iter)) {
	struct route_table_entry_t *entry = iter.current;
	struct route_t *route = entry->route;
	for (u16 i = 0; i < route->path_length; i++) {
	    if (route->path[i] == invalid_node_id) {
		/* remove route from routing table */
		entry = remove_entry(node->routing_table, entry);
		/* free route */
		route_entry_free(entry);
		break;
	    }
	}
	iter_next(&iter);
    }
}

void remove_old_neighbors(struct node_t *node)
{
    // TODO: if high lock contention consider condition variable
    pthread_mutex_lock(&node->neighbor_list_lock);
    time_t now = time(NULL);

    double removed = 0;
    double new_neighbors = node->new_neighbors_count;
    double neighbor_count = 0;
    struct neighbor_t *neighbor = NULL;
    for (int i = 0; i < node->known_nodes_count; i++) {
	neighbor = node->neighbors[i];
	if (neighbor == NULL)
	    continue;

	neighbor_count++;
	if (now - neighbor->last_seen > node->remove_neighbor_threshold) {
	    // LOG_NODE_INFO(node->node_id, "%d removed as neighbor", i + 1);
	    node->neighbors[i] = NULL;
	    free(neighbor);
	    removed++;
	    // invalidate all routes that use this neighbor
	    remove_route_with_old_neighbor(node, i + 1);
	}
    }

    pthread_mutex_unlock(&node->neighbor_list_lock);

    node->new_neighbors_count = 0;

    neighbor_count -= new_neighbors;
    double removed_decimal = (removed / neighbor_count);
    double new_neighbors_decimal = (new_neighbors / neighbor_count);
    if (removed_decimal > 0.7 || new_neighbors_decimal > 0.5)
	remove_all_entries(node->routing_table);
}

/* node lifetime functions */
bool init_node(struct node_t *node, u16 node_id, u8 poll_interval, u8 remove_neighbor_threshold,
	       u16 known_nodes_count, u16 max_connections, u16 max_threads, u16 queue_size,
	       node_init_known_nodes_func_t init_known_ids_func,
	       node_can_connect_func_t can_connect_func, node_send_func_t send_func,
	       node_recv_func_t rec_func, u16 port)
{
    /* set node options */
    node->init_known_nodes_func = init_known_ids_func;
    node->can_connect_func = can_connect_func;
    node->send_func = send_func;
    node->recv_func = rec_func;
    node->hello_poll_interval = poll_interval;
    node->remove_neighbor_threshold = remove_neighbor_threshold;
    node->known_nodes_count = known_nodes_count;

    /* init socket to external connection from client */
    node->node_id = node_id;
    node->sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (node->sockfd < 0) {
	LOG_NODE_ERR(node->node_id, "ABORT!: Failed to create socket");
	return false;
    }

#ifdef __APPLE__
    int optname = SO_REUSEADDR;
#else
    int optname = SO_REUSEADDR | 15;
#endif

    if (setsockopt(node->sockfd, SOL_SOCKET, optname, &(int){ 1 }, sizeof(int)) < 0) {
	LOG_NODE_ERR(node->node_id, "ABORT!: Failed to set socket options");
	return false;
    }

    struct sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(node->sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
	LOG_NODE_ERR(node->node_id, "ABORT!: Failed to bind socket");
	return false;
    }

    if (listen(node->sockfd, max_connections) != 0) {
	LOG_NODE_ERR(node->node_id, "ABORT!: Failed listen on socket");
	return false;
    }

    /* init routing table */
    node->routing_table = malloc(sizeof(struct route_table_t));

    node->connections = malloc(sizeof(struct connections_t));
    node->connections->connections = calloc(max_connections, sizeof(int));
    node->connections->index = -1;
    node->connections->cap = max_connections;

    /* init threadpool */
    node->threadpool = malloc(sizeof(struct threadpool_t));
    init_threadpool(node->threadpool, max_threads, queue_size);

    /* init neighbor list */
    node->neighbors = calloc(node->known_nodes_count, sizeof(struct neighbor_t *));
    for (int i = 0; i < node->known_nodes_count; i++) {
	node->neighbors[i] = NULL;
    }
    pthread_mutex_init(&node->neighbor_list_lock, NULL);

    /* known id list */
    node->known_nodes = malloc(sizeof(struct u16_arraylist_t));
    ARRAY_INIT(node->known_nodes);
    node->init_known_nodes_func(node);

    node->new_neighbors_count = 0;

    // LOG_NODE_INFO(node->node_id, "Successfully initialized");
    return true;
}

int run_node(struct node_t *node)
{
    /* start the nodes internal threadpool */
    start_threadpool(node->threadpool);
    node->running = true;

    submit_worker_task(node->threadpool, main_recv_thread, (void *)node);
    submit_worker_task(node->threadpool, hello_poll_thread, (void *)node);

    LOG_NODE_INFO(node->node_id, "Successfully started");

    fd_set readfds;
    struct timeval timeout;

    /*
     * polling for external connection from client.
     * if client connects during the "sleeping" phase, the "sleeping" is woken up early in
     * order to instantly handled the incoming external request.
     */
    while (node->running) {
	FD_ZERO(&readfds);
	FD_SET(node->sockfd, &readfds);
	/* set polling timeout to 1/100th of a second */
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;

	int ready = select(node->sockfd + 1, &readfds, NULL, NULL, &timeout);
	if (!(ready > 0 && FD_ISSET(node->sockfd, &readfds))) {
	    continue;
	}

	int client_sockfd = accept(node->sockfd, NULL, NULL);
	if (client_sockfd != -1) {
	    insert_external_connection(node->connections, client_sockfd);
	    LOG_INFO("Inserted external connection");

	    struct external_request_thread_data_t *data =
		malloc(sizeof(struct external_request_thread_data_t));
	    data->connection = client_sockfd;
	    data->node = node;

	    submit_worker_task(node->threadpool, handle_external, (void *)data);
	}
    }
    return 0;
}

void close_node(struct node_t *node)
{
    node->running = false;
    close_all_external_connections(node->connections);
    threadpool_stop(node->threadpool);
    close(node->sockfd);
    pthread_mutex_destroy(&node->neighbor_list_lock);
    LOG_NODE_INFO(node->node_id, "Shutdown complete");
}

void free_node(struct node_t *node)
{
    if (node->connections != NULL) {
	if (node->connections->connections != NULL)
	    free(node->connections->connections);
	free(node->connections);
    }

    if (node->threadpool != NULL) {
	free_threadpool(node->threadpool);
	free(node->threadpool);
    }

    if (node->neighbors != NULL) {
	for (int i = 0; i < node->known_nodes_count; i++) {
	    if (node->neighbors[i] != NULL)
		free(node->neighbors[i]);
	}
	free(node->neighbors);
    }

    if (node->routing_table != NULL) {
	route_table_free(node->routing_table);
    }

    if (node->known_nodes != NULL) {
	ARRAY_FREE(node->known_nodes);
	free(node->known_nodes);
    }
}

void destroy_node(struct node_t *node)
{
    close_node(node);
    free_node(node);
}
