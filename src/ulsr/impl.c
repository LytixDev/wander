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

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/logger.h"
#include "lib/threadpool.h"
#include "ulsr/impl.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/ulsr.h"


bool running;
struct node_t nodes[MESH_NODE_COUNT];
struct ulsr_internal_packet packet_limbo[MESH_NODE_COUNT];
struct await_t node_locks[MESH_NODE_COUNT];
struct simulation_coord_t coords[MESH_NODE_COUNT];


static void sleep_microseconds(unsigned int microseconds)
{
    struct timeval tv;
    tv.tv_sec = microseconds / 1000000;
    tv.tv_usec = microseconds % 1000000;
    select(0, NULL, NULL, NULL, &tv);
}

static void run_node_stub(void *arg)
{
    run_node((struct node_t *)arg);
}

static void init_coords()
{
    /* defaults */
    if (MESH_NODE_COUNT == 8) {
	coords[0].x = 50;
	coords[0].y = 50;

	coords[1].x = 225;
	coords[1].y = 150;

	coords[2].x = 250;
	coords[2].y = 275;

	coords[3].x = 300;
	coords[3].y = 400;

	coords[4].x = 425;
	coords[4].y = 275;

	coords[5].x = 400;
	coords[5].y = 400;

	coords[6].x = 550;
	coords[6].y = 450;

	coords[7].x = 600;
	coords[7].y = 50;

    } else {
	/* random */
    }
}

u16 distance(struct simulation_coord_t *a, struct simulation_coord_t *b)
{
    // some yolo type conversions here
    double delta_x = b->x - a->x;
    double delta_y = b->y - a->y;
    double d = sqrt(delta_x * delta_x + delta_y * delta_y);
    return (u16)d;
}

void set_initial_node_ids(struct node_t *node)
{
    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	ARRAY_PUSH(node->known_ids, (u16)(i + 1));
    }
}

u16 send_func(struct ulsr_internal_packet *packet, u16 node_id)
{
    // the mock
    if (distance(&coords[packet->prev_node_id - 1], &coords[node_id - 1]) > SIMULATION_NODE_RANGE)
	return 0;

    pthread_mutex_lock(&node_locks[node_id - 1].cond_lock);

    packet_limbo[node_id - 1] = *packet;

    pthread_cond_signal(&node_locks[node_id - 1].cond_variable);
    pthread_mutex_unlock(&node_locks[node_id - 1].cond_lock);

    return packet->payload_len;
};

struct ulsr_internal_packet *recv_func(u16 node_id)
{
    u16 node_idx = node_id - 1;
    pthread_mutex_lock(&node_locks[node_idx].cond_lock);

    while (packet_limbo[node_id - 1].type == PACKET_NONE && running)
	pthread_cond_wait(&node_locks[node_idx].cond_variable, &node_locks[node_idx].cond_lock);
    pthread_mutex_unlock(&node_locks[node_idx].cond_lock);

    if (packet_limbo[node_id - 1].type == PACKET_NONE)
	return NULL;

    /* consume the packet */
    struct ulsr_internal_packet *packet = malloc(sizeof(struct ulsr_internal_packet));
    *packet = packet_limbo[node_idx];
    packet_limbo[node_idx] = (struct ulsr_internal_packet){ 0 };
    packet_limbo[node_idx].type = PACKET_NONE;

    return packet;
}

bool simulate(void)
{
    /* mock distance */
    init_coords();

    /* send and recv implementations declared in ulsr/impl.h and defined in uslr/impl.c */
    node_send_func_t node_send_func = send_func;
    node_recv_func_t node_recv_func = recv_func;

    /* init all packet limbos to be none */
    for (int i = 0; i < MESH_NODE_COUNT; i++)
	packet_limbo[i].type = PACKET_NONE;

    /* main threadpool */
    struct threadpool_t threadpool;
    init_threadpool(&threadpool, MESH_NODE_COUNT + 1, 8);
    start_threadpool(&threadpool);

    running = true;

    /* init all nodes and make them run on the threadpool */
    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	int rc = init_node(&nodes[i], i + 1, 8, 8, 8, node_send_func, node_recv_func,
			   ULSR_DEVICE_PORT_START + i);
	if (rc == -1)
	    exit(1);

	submit_worker_task(&threadpool, run_node_stub, &nodes[i]);
    }

    while (running)
	running = (getc(stdin) != 'q');
    ;

    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	pthread_mutex_lock(&node_locks[i].cond_lock);
	pthread_cond_signal(&node_locks[i].cond_variable);
	pthread_mutex_unlock(&node_locks[i].cond_lock);
    }

    LOG_INFO("Stopping MAIN threadpool");

    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	close_node(&nodes[i]);
	free_node(&nodes[i]);
    }

    threadpool_stop(&threadpool);
    free_threadpool(&threadpool);
    return 0;
}
