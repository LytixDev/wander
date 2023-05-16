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

#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "lib/common.h"
#include "lib/logger.h"
#include "ulsr/packet.h"
#include "ulsr/ulsr.h"

#define MY_IP "127.0.0.1"

#define MESH_DEVICE_IP "127.0.0.1"
#define MESH_DEVICE_PORT 8087

/*
 * I want to send an HTTP GET request to http://datakom.no
 */
void construct_test_packet(struct ulsr_packet *p)
{
    /* datakom.no IP */
    char host[16] = "20.100.42.130";

    /* payload */
    char *payload = "GET / HTTP/1.1\r\n"
		    "Host: datakom.no\r\n"
		    "Connection: close\r\n"
		    "\r\n";
    u16 payload_len = strlen(payload);

    /* construct packet */
    p->type = ULSR_HTTP;
    strncpy(p->source_ipv4, MY_IP, 16);
    strncpy(p->dest_ipv4, host, 16);
    p->dest_port = 80;
    p->payload_len = payload_len;
    strncpy((char *)p->payload, payload, payload_len);
}

void send_test_packet()
{
    struct ulsr_packet packet;
    construct_test_packet(&packet);

    /* create socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
	LOG_ERR("Socker creation failed\n");
	exit(1);
    }

    LOG_INFO("Client socket successfully created\n");
    struct sockaddr_in servaddr = { 0 };

    /* assign IP and PORT */
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(MESH_DEVICE_IP);
    servaddr.sin_port = htons(MESH_DEVICE_PORT);

    /* connect the client socket to mesh device */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
	LOG_ERR("Could not connect client to mesh device");
	exit(1);
    }

    LOG_INFO("Successfully connected to the mesh device");

    /* send */
    ssize_t sent = send(sockfd, &packet, sizeof(struct ulsr_packet), 0);
    if (sent == -1)
	LOG_ERR("could not send :-(");
    else if (sent == sizeof(struct ulsr_packet))
	LOG_INFO("probably good?");

    close(sockfd);
}

void listen_for_response()
{
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
	LOG_ERR("Socker creation failed\n");
	exit(1);
    }

    LOG_INFO("Created socket");
    struct sockaddr_in address = { 0 };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(ULSR_DEFAULT_PORT);

    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
	LOG_ERR("Failed to bind socket");
	exit(1);
    }

    LOG_INFO("Successfully binded socket");

    if ((listen(sockfd, 4)) != 0) {
	LOG_ERR("Listen failed");
	exit(1);
    }

    LOG_INFO("Server listening");

    struct sockaddr_in client = { 0 };
    unsigned int len = sizeof(client);
    int connfd = accept(sockfd, (struct sockaddr *)&client, &len);
    if (connfd < 0) {
	LOG_ERR("Server accept failed");
	exit(1);
    }


    while (1) {
        struct ulsr_packet packet;
        ssize_t received = recv(connfd, &packet, sizeof(struct ulsr_packet), 0);
        if (received <= 0) {
            LOG_INFO("Done receiving");
            break;
        }

        LOG_INFO("%s", packet.payload);
    }

    close(connfd);
    close(sockfd);
}

int main(void)
{
    /*
     * 1. construct ulsr packet
     * 2. connect to a node of the mesh
     * 3. send the packet to the mesh
     * 4. wait for response
     * 5. print response
     */

    send_test_packet();
    listen_for_response();

    /*
     * realistic flow:
     *
     * while (running) {
     *    packet decide_packet()
     *    send_packet(packet)
     *    listen_for_response()
     * }
     */
}
