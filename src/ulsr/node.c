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

static bool handle_send_external(struct node_t *node, struct ulsr_internal_packet *packet)
{
    struct ulsr_packet *internal_payload = packet->payload;
    LOG_NODE_INFO(node->node_id, "Handling outgoing request to IP %s at port %d",
		  internal_payload->dest_ipv4, internal_payload->dest_port);

    int ext_sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (ext_sockfd < 0) {
	LOG_NODE_ERR(node->node_id, "Failed to create socket");
	return false;
    }

    LOG_NODE_INFO(node->node_id, "Created socket");

    struct sockaddr_in server = { 0 };
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(internal_payload->dest_ipv4);
    server.sin_port = htons(internal_payload->dest_port);

    if (connect(ext_sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
	LOG_NODE_ERR(node->node_id, "Failed to connect to %s/%d", internal_payload->dest_ipv4,
		     internal_payload->dest_port);
	return false;
    }

    LOG_NODE_INFO(node->node_id, "Connected to %s/%d", internal_payload->dest_ipv4,
		  internal_payload->dest_port);

    if (send(ext_sockfd, internal_payload->payload, internal_payload->payload_len, 0) < 0) {
	LOG_NODE_ERR(node->node_id, "Failed to send packet to %s/%d", internal_payload->dest_ipv4,
		     internal_payload->dest_port);
	return false;
    }

    LOG_NODE_INFO(node->node_id, "Sent packet to %s/%d", internal_payload->dest_ipv4,
		  internal_payload->dest_port);

    u8 response[1024] = { 0 };
    // REMOVE THIS IN FUTURE, THIS IS NOT A GOOD WAY TO DO THIS AND DOES NOT WORK WITH MULTIPLE
    // NODES
    int recv_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (recv_socket < 0) {
	LOG_NODE_ERR(node->node_id, "Failed to create return socket");
	return false;
    }

    struct sockaddr_in recv_server = { 0 };
    recv_server.sin_family = AF_INET;
    recv_server.sin_addr.s_addr = inet_addr(internal_payload->source_ipv4);
    recv_server.sin_port = htons(ULSR_DEFAULT_PORT);

    if (connect(recv_socket, (struct sockaddr *)&recv_server, sizeof(recv_server)) < 0) {
	LOG_NODE_ERR(node->node_id, "Failed to create connect to socket");
	return false;
    }

    LOG_NODE_INFO(node->node_id, "Connected to return socket");

    u32 seq_nr = 0;
    while (node->running && recv(ext_sockfd, response, 1024 - 1, 0) > 0) {
	struct ulsr_packet ret_packet = { 0 };
	strncpy(ret_packet.source_ipv4, internal_payload->dest_ipv4, 16);
	strncpy(ret_packet.dest_ipv4, internal_payload->source_ipv4, 16);
	ret_packet.dest_port = ULSR_DEFAULT_PORT;
	ret_packet.payload_len = strlen((char *)response);
	strncpy((char *)ret_packet.payload, (const char *)response, ret_packet.payload_len);
	ret_packet.type = ULSR_HTTP;
	ret_packet.seq_nr = seq_nr;

	if (send(recv_socket, &ret_packet, sizeof(ret_packet), 0) < 0) {
	    LOG_NODE_ERR(node->node_id, "Failed to send return packet");
	    return false;
	}
	LOG_NODE_INFO(node->node_id, "Sent return packet with seq_nr %d", seq_nr);
	memset(response, 0, 1024);
	seq_nr++;
    }

    close(recv_socket);
    return true;
}

static void handle_send_internal(void *arg)
{
    struct node_t *node = (struct node_t *)arg;

    struct ulsr_internal_packet *packet = NULL;

    while (node->running) {
	packet = node->rec_func(node->node_id);
	if (packet != NULL) {
	    struct ulsr_packet *payload = packet->payload;
	    LOG_NODE_INFO(node->node_id, "Received packet from rec_func");
	    LOG_NODE_INFO(node->node_id, "Received packet");
	    LOG_NODE_INFO(node->node_id, "Source: %s", payload->source_ipv4);
	    LOG_NODE_INFO(node->node_id, "Destination: %s", payload->dest_ipv4);
	    LOG_NODE_INFO(node->node_id, "Payload: %s", payload->payload);

	    //     if (payload->dest_ipv4 == node->node_id) {
	    if (node->node_id == 2) {
		LOG_NODE_INFO(node->node_id, "Packet is for this node");
		handle_send_external(node, packet);
	    } else {
		LOG_NODE_INFO(node->node_id, "Packet is not for this node");

		// This is a hack, but it works for now as we only have 2 nodes
		if (node->node_id < 2)
		    node->send_func(packet, node->node_id + 1);
	    }

	    free(packet);
	}
    }
}

static void handle_external(void *arg)
{
    struct external_request_thread_data_t *data = (struct external_request_thread_data_t *)arg;
    struct node_t *node = data->node;
    struct ulsr_packet packet = { 0 };

    ssize_t bytes_read = recv(data->connection, &packet, sizeof(struct ulsr_packet), 0);
    if (bytes_read <= 0) {
	LOG_NODE_ERR(node->node_id, "ABORT!: Failed to read from socket");
	goto cleanup;
    }

    LOG_NODE_INFO(node->node_id, "Received external packet:");
    LOG_NODE_INFO(node->node_id, "External packet source: %s", packet.source_ipv4);
    LOG_NODE_INFO(node->node_id, "External packet destination: %s", packet.dest_ipv4);
    LOG_NODE_INFO(node->node_id, "External payload: %s", packet.payload);

    /* validate checksum */
    u32 checksum = ulsr_checksum((u8 *)&packet, (unsigned long)bytes_read);
    if (checksum != packet.checksum) {
	LOG_NODE_ERR(node->node_id, "ABORT!: Checksum failed!");
	goto cleanup;
    }

    /* pack external packet into internal packet for routing between nodes */
    struct ulsr_internal_packet *internal_packet = ulsr_internal_from_external(&packet);
    internal_packet->prev_node_id = data->node->node_id;
    // TEMP HACK
    internal_packet->dest_node_id = 2;
    // internal_packet->dest_node_id = find_path(data->node_id, packet.dest_ipv4);

    /* add path to send func */
    if (internal_packet->dest_node_id == data->node->node_id) {
	if (!handle_send_external(data->node, internal_packet)) {
	    LOG_NODE_ERR(node->node_id, "ABORT!: Failed to handle sending of external request");
	    goto cleanup;
	}
    } else {
	data->node->send_func(internal_packet, data->node->node_id);
    }

    free(internal_packet);

cleanup:
    shutdown(data->connection, SHUT_RDWR);
    close(data->connection);
    free(data);
    LOG_NODE_INFO(node->node_id, "Closed connection");
}

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

    LOG_NODE_INFO(node->node_id, "Succesfully created socket");

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

    LOG_NODE_INFO(node->node_id, "Succesfully bound socket");

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

    LOG_NODE_INFO(node->node_id, "Completed initialization");
    return true;
}

int run_node(struct node_t *node)
{
    /* start the nodes internal threadpool */
    start_threadpool(node->threadpool);
    node->running = true;

    submit_worker_task(node->threadpool, handle_send_internal, (void *)node);

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
	ARRAY_FREE(*(node->neighbors));
    }
}
