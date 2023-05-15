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

#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ulsr/node.h"
#include "lib/logger.h"
#include "lib/common.h"

int init_node(struct node_t *node, int connections, int threads, int queue_size, ...)
{
    node->socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (node->socket < 0) {
        LOG_ERR("Failed to create socket");
        return -1;
    }

    if (setsockopt(node->socket, SOL_SOCKET, IP_HDRINCL, &(int){1}, sizeof(int)) < 0) {
        LOG_ERR("Failed to set socket options");
        return -1;
    }

    node->connections = malloc(sizeof(struct connections_t));
    node->connections->connections = malloc(sizeof(int) * connections);
    node->connections->cap = connections;

    node->threadpool = malloc(sizeof(struct threadpool_t));
    init_threadpool(node->threadpool, threads, queue_size);

    node->all_nodes = malloc(sizeof(struct sockaddr_in_array_t));

    va_list args;
    u16 num_args = VA_NUMBER_OF_ARGS(args);
    va_start(args, num_args);

    for (int i = 0; i < num_args; i++)
        ARRAY_PUSH(*node->all_nodes, va_arg(args, struct sockaddr_in));

    va_end(args);

    return 0;
}