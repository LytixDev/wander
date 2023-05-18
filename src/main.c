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

#include <stdio.h>

#include "lib/common.h"
#include "lib/logger.h"
#include "lib/threadpool.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/ulsr.h"

#define MESH_NODE_COUNT 2

struct node_t nodes[MESH_NODE_COUNT];
struct ulsr_internal_packet packet_limbo[MESH_NODE_COUNT];


static void check_quit(void *arg)
{
    while (getc(stdin) != 'q')
	;

    LOG_INFO("Quitting");

    bool *running = (bool *)arg;
    *running = false;
}

u16 send_func(struct ulsr_internal_packet *packet, u16 node_id)
{
    packet_limbo[node_id] = *packet;
    return packet->payload_len;
};

struct ulsr_internal_packet *recv_func(u16 node_id)
{
    if (packet_limbo[node_id - 1].type == PACKET_NONE)
	return NULL;

    struct ulsr_internal_packet *packet = malloc(sizeof(struct ulsr_internal_packet));
    *packet = packet_limbo[node_id - 1];
    packet_limbo[node_id - 1] = (struct ulsr_internal_packet){ 0 };
    packet_limbo[node_id - 1].type = PACKET_NONE;
    return packet;
}

void run_node_stub(void *arg)
{
    run_node((struct node_t *)arg);
}

int main(void)
{
    /* send and recv implementations */
    node_send_func_t node_send_func = send_func;
    node_recv_func_t node_recv_func = recv_func;

    /* init all packet limbos to be none */
    for (int i = 0; i < MESH_NODE_COUNT; i++)
	packet_limbo[i].type = PACKET_NONE;

    /* main threadpool */
    struct threadpool_t threadpool;
    init_threadpool(&threadpool, MESH_NODE_COUNT, 8);
    start_threadpool(&threadpool);

    /* init all nodes and make them run on the threadpool */
    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	int rc = init_node(&nodes[i], i + 1, 8, 8, 8, node_send_func, node_recv_func, NULL, NULL,
			   ULSR_DEVICE_PORT_START + i);
	if (rc == -1)
	    exit(1);

	submit_worker_task(&threadpool, run_node_stub, &nodes[i]);
    }

    while ((nodes[0].running == true || nodes[1].running == true))
	;

    bool running = true;
    submit_worker_task(&threadpool, check_quit, &running);
    while (running == true)
	;

    LOG_INFO("Stopping threadpool... FOR MAIN");

    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	free_node(&nodes[i]);
    }

    threadpool_stop(&threadpool);

    free_threadpool(&threadpool);
    return 0;
}
