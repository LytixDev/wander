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

#include "ulsr/node.h"
#include "lib/common.h"

int init_node(struct node_t *node, int connections, int threads, int queue_size, ...)
{
    node->socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (node->socket < 0) {
        perror("socket");
        return -1;
    }

    if (setsockopt(node->socket, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt");
        return -1;
    }
    
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server->socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(server->socket, 5) != 0) {
        perror("listen");
        return -1;
    }

    server->connections = malloc(sizeof(struct connections_t));
    server->connections->connections = malloc(sizeof(int) * connections);
    server->connections->cap = connections;

    server->threadpool = malloc(sizeof(struct threadpool_t));
    init_threadpool(server->threadpool, threads, q_size);

    server->routes = malloc(sizeof(struct routes_t));
    init_routes(server->routes);

    va_list args;
    u16 num_args = VA_NUMBER_OF_ARGS(args);
    va_start(args, num_args);

    for (int i = 0; i < num_args; i++) {
        // TODO: retrieve the next argument from the va_list
        // and initialize it
    }

    va_end(args);

    return 0;
}