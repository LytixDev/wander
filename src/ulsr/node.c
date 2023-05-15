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
#include <stdlib.h>
#include <sys/socket.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/lambda.h"
#include "lib/logger.h"
#include "lib/socket_utils.h"
#include "ulsr/node.h"

static void close_connections(struct connections_t *connections)
{
    LOG_INFO("Closing connections");
    for (int i = 0; i < connections->cap; i++) {
	if (send(connections->connections[i], "Server shutting down", 20, MSG_NOSIGNAL) > 0) {
	    shutdown(connections->connections[i], SHUT_RDWR);
	    close(connections->connections[i]);
	}
    }
    LOG_INFO("Closed connections");
}

static void insert_connection(struct connections_t *connections, int connection)
{
    connections->index = ++connections->index % connections->cap;
    connections->connections[connections->index] = connection;
    LOG_INFO("Inserted connection");
}

static void check_quit(void *arg)
{
    struct node_t *node = (struct node_t *)arg;

    while (getc(stdin) != 'q')
	;

    shutdown(node->socket, SHUT_RDWR);
    close(node->socket);
    LOG_INFO("Quitting...");

    node->running = false;
}

int init_node(struct node_t *node, int connections, int threads, int queue_size, ...)
{
    node->socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (node->socket < 0) {
	LOG_ERR("Failed to create socket");
	return -1;
    }

    LOG_INFO("Created socket");

    if (setsockopt(node->socket, SOL_SOCKET, IP_HDRINCL, &(int){ 1 }, sizeof(int)) < 0) {
	LOG_ERR("Failed to set socket options");
	return -1;
    }

    LOG_INFO("Set socket options");

    struct sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(ULSR_DEFAULT_PORT);

    if (bind(node->socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
	LOG_ERR("Failed to bind socket");
	return -1;
    }

    if (listen(node->socket, threads) != 0) {
	LOG_ERR("Failed to listen on socket");
	return -1;
    }

    LOG_INFO("Bound socket");

    node->connections = malloc(sizeof(struct connections_t));
    node->connections->connections = calloc(connections, sizeof(int));
    node->connections->index = -1;
    node->connections->cap = connections;

    LOG_INFO("Allocated connections");

    node->threadpool = malloc(sizeof(struct threadpool_t));
    init_threadpool(node->threadpool, threads, queue_size);

    node->all_nodes = malloc(sizeof(struct sockaddr_in_array_t));

    va_list args;
    u16 num_args = VA_NUMBER_OF_ARGS(args);
    va_start(args, num_args);

    for (int i = 0; i < num_args; i++)
	ARRAY_PUSH(*node->all_nodes, va_arg(args, struct sockaddr_in));

    va_end(args);

    LOG_INFO("Initialized node");

    return 0;
}

int run_node(struct node_t *node)
{
    int client_socket = -1;

    start_threadpool(node->threadpool);

    LOG_INFO("Started threadpool");

    node->running = true;

    LOG_INFO("Server started, press 'q' to quit\n");

    submit_worker_task(node->threadpool, check_quit, (void *)node);

    set_nonblocking(node->socket);

    while (node->running) {
	client_socket = accept(node->socket, NULL, NULL);

	if (client_socket != -1) {
	    insert_connection(node->connections, client_socket);

	    LOG_INFO("Connection with client %d established", client_socket);

	    // TODO: Handle connection
	    client_socket = -1;
	}
    }

    LOG_INFO("Closing connections");

    close_connections(node->connections);

    LOG_INFO("Stopping threadpool");

    threadpool_stop(node->threadpool);

    LOG_INFO("Freeing node");

    free_node(node);

    LOG_INFO("Shutdown complete");

    return 0;
}

void free_node(struct node_t *node)
{
    free(node->connections->connections);
    free(node->connections);
    free(node->threadpool);
    free(node->all_nodes);
}