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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/logger.h"
#include "ulsr/comms_external.h"
#include "ulsr/comms_internal.h"
#include "ulsr/impl.h"
#include "ulsr/node.h"


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

/* node lifetime functions */

bool init_node(struct node_t *node, u16 node_id, u16 connections, u16 threads, u16 queue_size,
	       node_can_connect_func_t can_connect_func, node_send_func_t send_func,
	       node_recv_func_t rec_func, u16 port)
{
    /* init socket to external connection from client */
    node->node_id = node_id;
    node->sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (node->sockfd < 0) {
	LOG_NODE_ERR(node->node_id, "ABORT!: Failed to create socket");
	return false;
    }

    if (setsockopt(node->sockfd, SOL_SOCKET, SO_REUSEADDR | 15, &(int){ 1 }, sizeof(int)) < 0) {
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

    if (listen(node->sockfd, threads) != 0) {
	LOG_NODE_ERR(node->node_id, "ABORT!: Failed listen on socket");
	return false;
    }

    node->connections = malloc(sizeof(struct connections_t));
    node->connections->connections = calloc(connections, sizeof(int));
    node->connections->index = -1;
    node->connections->cap = connections;

    /* init threadpool */
    node->threadpool = malloc(sizeof(struct threadpool_t));
    init_threadpool(node->threadpool, threads, queue_size);

    /* set node options */
    node->can_connect_func = can_connect_func;
    node->send_func = send_func;
    node->recv_func = rec_func;

    /* init route datastructure */
    node->route_queue = (struct queue_t *)(malloc(sizeof(struct queue_t)));
    init_queue(node->route_queue, MESH_NODE_COUNT);

    /* init neighbor list */
    node->neighbors = calloc(MESH_NODE_COUNT, sizeof(struct neighbor_t *));
    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	node->neighbors[i] = NULL;
    }

    /* known id list */
    node->known_ids = malloc(sizeof(struct u16_arraylist_t));
    ARRAY_INIT(node->known_ids);
    set_initial_node_ids(node);

    LOG_NODE_INFO(node->node_id, "Successfully initialized");
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
	for (int i = 0; i < MESH_NODE_COUNT; i++) {
	    if (node->neighbors[i] != NULL)
		free(node->neighbors[i]);
	}
	free(node->neighbors);
    }

    if (node->route_queue != NULL) {
	free_queue(node->route_queue);
	free(node->route_queue);
    }

    if (node->known_ids != NULL) {
	ARRAY_FREE(node->known_ids);
	free(node->known_ids);
    }
}
