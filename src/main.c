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
#include "lib/lambda.h"
#include "lib/threadpool.h"
#include "lib/logger.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/ulsr.h"

#define MESH_NODE_COUNT 1

static void check_quit(void *arg)
{
    while (getc(stdin) != 'q')
	;

    LOG_INFO("Quitting...");

    bool *running = (bool *)arg;
    *running = false;
}

int main(void)
{
    struct node_t nodes[MESH_NODE_COUNT];
    struct ulsr_internal_packet packet_limbo[MESH_NODE_COUNT] = { 0 };
    struct threadpool_t threadpool = { 0 };
    init_threadpool(&threadpool, MESH_NODE_COUNT + 1, 8);

    start_threadpool(&threadpool);

    bool running = true;

    submit_worker_task(&threadpool, check_quit, &running);

    struct node_t node_one = { 0 };

    node_send_func_t node_send_func =
	LAMBDA(u16, (struct ulsr_internal_packet * packet, u16 node_id), {
	    packet_limbo[node_id] = *packet;
	    return packet->payload_len;
	});

    node_rec_func_t node_rec_func = LAMBDA(struct ulsr_internal_packet *, (u16 node_id), {
	struct ulsr_internal_packet *packet = malloc(sizeof(struct ulsr_internal_packet));
	*packet = packet_limbo[node_id - 1];
	packet_limbo[node_id - 1] = (struct ulsr_internal_packet){ 0 };
	packet_limbo[node_id - 1].pt = PACKET_NONE;
	return packet;
    });

    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	int rc = init_node(&nodes[i], i + 1, 8, 8, 8, NULL, node_send_func, node_rec_func, NULL,
			   NULL, ULSR_DEVICE_PORT_START + i);
	if (rc == -1)
	    exit(1);

	submit_worker_task(
	    &threadpool, LAMBDA(void, (void *arg), { run_node((struct node_t *)arg); }), &nodes[i]);
    }

    while (nodes[0].running = true || nodes[1].running == true && running == true);

    LOG_INFO("Stopping threadpool... FOR MAIN");
    threadpool_stop(&threadpool);

    free_threadpool(&threadpool);

    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	free_node(&nodes[i]);
    }


    return 0;
}
