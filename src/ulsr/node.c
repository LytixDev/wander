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
#include <string.h>
#include <sys/socket.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/logger.h"
#include "lib/socket_utils.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/ulsr.h"

struct external_request_thread_data_t {
    u16 connection;
    struct node_t *node;
};

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

static void check_quit(void *arg)
{
    struct node_t *node = (struct node_t *)arg;

    while (getc(stdin) != 'q')
	;

    shutdown(node->sockfd, SHUT_RDWR);
    close(node->sockfd);
    LOG_INFO("Quitting...");

    node->running = false;
}

static int handle_send_external_request(struct node_t *node, struct ulsr_internal_packet *packet)
{
    LOG_INFO("Handling request to send external ip and node id: %d", node->node_id);
    struct ulsr_packet *internal_payload = packet->payload;

    int ext_sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (ext_sockfd < 0) {
	LOG_ERR("Failed to create socket");
	return -1;
    }

    LOG_INFO("Created socket");

    struct sockaddr_in server = { 0 };
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(internal_payload->dest_ipv4);
    server.sin_port = htons(internal_payload->dest_port);

    if (connect(ext_sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
	LOG_ERR("Failed to connect to external IP server");
	return -1;
    }

    LOG_INFO("Connected to server");

    if (send(ext_sockfd, internal_payload->payload, internal_payload->payload_len, 0) < 0) {
	LOG_ERR("Failed to send packet to external IP");
	return -1;
    }

    LOG_INFO("Sent packet to external IP");

    u8 response[1024] = { 0 };

    // REMOVE THIS IN FUTURE, THIS IS NOT A GOOD WAY TO DO THIS AND DOES NOT WORK WITH MULTIPLE
    // NODES

    int rec_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (rec_socket < 0) {
	LOG_ERR("Failed to create return socket");
	return -1;
    }

    struct sockaddr_in rec_server = { 0 };
    rec_server.sin_family = AF_INET;
    rec_server.sin_addr.s_addr = inet_addr(internal_payload->source_ipv4);
    rec_server.sin_port = htons(ULSR_DEFAULT_PORT);

    if (connect(rec_socket, (struct sockaddr *)&rec_server, sizeof(rec_server)) < 0) {
	LOG_ERR("Failed to connect to return socket");
	return -1;
    }

    LOG_INFO("Connected to return socket");

    while (node->running && recv(ext_sockfd, response, 1024 - 1, 0) > 0) {
	struct ulsr_packet ret_packet = { 0 };
	strncpy(ret_packet.source_ipv4, internal_payload->dest_ipv4, 16);
	strncpy(ret_packet.dest_ipv4, internal_payload->source_ipv4, 16);
	ret_packet.dest_port = ULSR_DEFAULT_PORT;
	ret_packet.payload_len = strlen(response);
	strncpy(ret_packet.payload, response, ret_packet.payload_len);
	ret_packet.type = ULSR_HTTP;

	if (send(rec_socket, &ret_packet, sizeof(ret_packet), 0) < 0) {
	    LOG_ERR("Failed to send packet");
	    return -1;
	}
	LOG_INFO("Sent packet");
	memset(response, 0, 1024);
    }

    close(rec_socket);
}

static void handle_send_request(void *arg)
{
    struct node_t *node = (struct thread_data_t *)arg;

    struct ulsr_internal_packet *packet = NULL;

    while (node->running) {
	packet = node->rec_func(node->node_id);
	if (packet != NULL) {
	    struct ulsr_packet *payload = packet->payload;
	    LOG_INFO("Received packet from rec_func");
	    LOG_INFO("Received packet");
	    LOG_INFO("Source: %s", payload->source_ipv4);
	    LOG_INFO("Destination: %s", payload->dest_ipv4);
	    LOG_INFO("Payload: %s", payload->payload);

	    //     if (payload->dest_ipv4 == node->node_id) {
	    if (node->node_id == 2) {
		LOG_INFO("Packet is for this node");
		handle_send_external_request(node, packet);
	    } else {
		LOG_INFO("Packet is not for this node");

		// This is a hack, but it works for now as we only have 2 nodes
		if (node->node_id < 2)
		    node->send_func(&packet, node->node_id + 1);
	    }
	}
    }
}

static void handle_external_request(void *arg)
{
    struct external_request_thread_data_t *data = (struct external_request_thread_data_t *)arg;

    struct ulsr_packet packet = { 0 };

    ssize_t bytes_read = recv(data->connection, &packet, sizeof(struct ulsr_packet), 0);

    if (bytes_read <= 0) {
	LOG_ERR("Failed to read from socket");
    } else {
	LOG_INFO("Received external packet");
	LOG_INFO("External packet source: %s", packet.source_ipv4);
	LOG_INFO("External packet destination: %s", packet.dest_ipv4);
	LOG_INFO("External payload: %s", packet.payload);

	u32 checksum = ulsr_checksum((u8 *)&packet, (unsigned long)bytes_read);
	if (checksum != packet.checksum) {
	    LOG_ERR("Checksum failed!");
	    goto cleanup;
	}
	LOG_INFO("Checksum correct");

	struct ulsr_internal_packet *internal_packet = ulsr_internal_packet_new(&packet);
	LOG_INFO("DONE WITH EXTERNAL PACKET INPUT");
	internal_packet->prev_node_id = data->node->node_id;

	internal_packet->dest_node_id = 2;
	// internal_packet->dest_node_id = find_path(data->node_id, packet.dest_ipv4);

	// Add path to send func
	if (internal_packet->dest_node_id == data->node->node_id) {
	    if (handle_send_external_request(data->node, internal_packet) == -1) {
		LOG_ERR("Failed to handle sending of external request");
		goto cleanup;
	    }
	} else {
	    data->node->send_func(internal_packet, data->node->node_id);
	}
    }

cleanup:
    shutdown(data->connection, SHUT_RDWR);
    close(data->connection);

    free(data);

    LOG_INFO("Closed connection");
}

int init_node(struct node_t *node, u16 node_id, u16 connections, u16 threads, u16 queue_size,
	      node_distance_func_t distance_func, node_send_func_t send_func,
	      node_rec_func_t rec_func, void *data, data_free_func_t data_free_func, u16 port)
{
    node->node_id = node_id;

    node->sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (node->sockfd < 0) {
	LOG_ERR("Failed to create socket");
	return -1;
    }

    LOG_INFO("Created socket");

    if (setsockopt(node->sockfd, SOL_SOCKET, SO_REUSEADDR | 15, &(int){ 1 }, sizeof(int)) < 0) {
	LOG_ERR("Failed to set socket options");
	return -1;
    }

    LOG_INFO("Set socket options");

    struct sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(node->sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
	LOG_ERR("Failed to bind socket");
	return -1;
    }

    if (listen(node->sockfd, threads) != 0) {
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

    node->data_free_func = data_free_func;
    node->data = data;

    node->distance_func = distance_func;
    node->send_func = send_func;
    node->rec_func = rec_func;

    LOG_INFO("Initialized node");

    return 0;
}

int run_node(struct node_t *node)
{
    int client_socket = -1;

    start_threadpool(node->threadpool);

    LOG_INFO("Started threadpool");

    node->running = true;

    LOG_INFO("Node started, press 'q' to quit\n");

    submit_worker_task(node->threadpool, check_quit, (void *)node);

    set_nonblocking(node->sockfd);

    submit_worker_task(node->threadpool, handle_send_request, (void *)node);

    while (node->running) {
	client_socket = accept(node->sockfd, NULL, NULL);

	if (client_socket != -1) {
	    insert_connection(node->connections, client_socket);

	    struct external_request_thread_data_t *data =
		malloc(sizeof(struct external_request_thread_data_t));
	    data->connection = client_socket;
	    data->node = node;

	    submit_worker_task(node->threadpool, handle_external_request, (void *)data);

	    client_socket = -1;
	}
    }

    LOG_INFO("Closing connections");

    close_connections(node->connections);

    LOG_INFO("Stopping threadpool");

    threadpool_stop(node->threadpool);

    close(node->sockfd);

    LOG_INFO("Shutdown complete");

    return 0;
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
	ARRAY_FREE(*(node->neighbors));
    }

    if (node->data != NULL) {
	node->data_free_func(node->data);
    }
}
