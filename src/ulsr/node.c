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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/logger.h"
#include "ulsr/ext_comms.h"
#include "ulsr/impl.h"
#include "ulsr/int_comms.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/route_table.h"
#include "ulsr/routing.h"
#include "ulsr/ulsr.h"


static void close_connections(struct connections_t *connections)
{
    LOG_INFO("Closing connections");
    for (int i = 0; i < connections->cap; i++) {
	if (send(connections->connections[i], "q", 2, MSG_NOSIGNAL) > 0) {
	    shutdown(connections->connections[i], SHUT_RDWR);
	    close(connections->connections[i]);
	}
    }
    LOG_INFO("Closed connections");
}

static void insert_connection(struct connections_t *connections, int connection)
{
    connections->index++;
    connections->index = connections->index % connections->cap;
    connections->connections[connections->index] = connection;
    LOG_INFO("Inserted connection");
}


/* node lifetime functions */
bool init_node(struct node_t *node, u16 node_id, u16 connections, u16 threads, u16 queue_size,
	       node_send_func_t send_func, node_recv_func_t rec_func, u16 port)
{
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

    // LOG_NODE_INFO(node->node_id, "Succesfully created socket");

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

    // LOG_NODE_INFO(node->node_id, "Succesfully bound socket");

    node->connections = malloc(sizeof(struct connections_t));
    node->connections->connections = calloc(connections, sizeof(int));
    node->connections->index = -1;
    node->connections->cap = connections;

    /* init threadpool */
    node->threadpool = malloc(sizeof(struct threadpool_t));
    init_threadpool(node->threadpool, threads, queue_size);

    /* set node options */
    node->send_func = send_func;
    node->rec_func = rec_func;

    /* init route table */
    route_table_init(&node->route_table);
    // LOG_NODE_INFO(node->node_id, "Succesfully initialized route table");

    /* init neighbor list */
    node->neighbors = malloc(sizeof(struct neighbor_t *) * MESH_NODE_COUNT);
    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	node->neighbors[i] = NULL;
    }

    /* known id list */
    node->known_ids = malloc(sizeof(struct u16_arraylist_t));
    ARRAY_INIT(node->known_ids);
    set_initial_node_ids(node);

    /* set initial node ids */
    LOG_NODE_INFO(node->node_id, "Completed initialization");
    return true;
}

int run_node(struct node_t *node)
{
    /* start the nodes internal threadpool */
    start_threadpool(node->threadpool);
    node->running = true;

    submit_worker_task(node->threadpool, handle_send_internal, (void *)node);
    submit_worker_task(node->threadpool, hello_poll_thread, (void *)node);

    LOG_NODE_INFO(node->node_id, "Node properly initialized");


    while (node->running) {
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(node->sockfd, &readfds);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;

	int ready = select(node->sockfd + 1, &readfds, NULL, NULL, &timeout);
	if (ready > 0 && FD_ISSET(node->sockfd, &readfds)) {
	    int client_sockfd = accept(node->sockfd, NULL, NULL);

	    if (client_sockfd != -1) {
		insert_connection(node->connections, client_sockfd);

		struct external_request_thread_data_t *data =
		    malloc(sizeof(struct external_request_thread_data_t));
		data->connection = client_sockfd;
		data->node = node;

		submit_worker_task(node->threadpool, handle_external, (void *)data);

		client_sockfd = -1;
	    }
	}
    }
    return 0;
}

void close_node(struct node_t *node)
{
    node->running = false;

    close_connections(node->connections);
    threadpool_stop(node->threadpool);
    close(node->sockfd);
    LOG_NODE_INFO(node->node_id, "Shutdown complete");
}

void free_node(struct node_t *node)
{
    if (node->connections != NULL) {
	if (node->connections->connections != NULL) {
	    free(node->connections->connections);
	}
	free(node->connections);
    }

    if (node->threadpool != NULL) {
	free_threadpool(node->threadpool);
    }

    if (node->neighbors != NULL) {
	for (int i = 0; i < MESH_NODE_COUNT; i++) {
	    if (node->neighbors[i] != NULL)
		free(node->neighbors[i]);
	}
	free(node->neighbors);
    }

    if (node->route_table != NULL) {
	route_table_free(node->route_table);
    }

    if (node->known_ids != NULL) {
	ARRAY_FREE(node->known_ids);
	free(node->known_ids);
    }
}
