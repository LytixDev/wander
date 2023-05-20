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

#ifdef GUI
#include "gui/window.h"
#endif
#include "lib/arraylist.h"
#include "lib/common.h"
#include "lib/logger.h"
#include "lib/queue.h"
#include "lib/threadpool.h"
#include "ulsr/impl.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/ulsr.h"


/* global simulation running variable */
static bool running;

#ifdef GUI
GLFWwindow *window;
struct threadpool_t window_threadpool;
#endif

struct node_t nodes[MESH_NODE_COUNT];
struct queue_t packet_limbo[MESH_NODE_COUNT];
struct await_t node_locks[MESH_NODE_COUNT];
struct simulation_coord_t coords[MESH_NODE_COUNT];
struct simulation_coord_t target_coords;
struct threadpool_t threadpool;

static void init_packet_limbo_queue()
{
    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	queue_init(&packet_limbo[i], 32);
    }
}

static void run_node_stub(void *arg)
{
    run_node((struct node_t *)arg);
}

static void init_coords()
{
    /* defaults */
    if (MESH_NODE_COUNT == 12) {
	coords[0].x = 100;
	coords[0].y = 100;

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

	coords[6].x = 175;
	coords[6].y = 325;

	coords[7].x = 325;
	coords[7].y = 200;

	coords[8].x = 400;
	coords[8].y = 500;

	coords[9].x = 500;
	coords[9].y = 400;

	coords[10].x = 350;
	coords[10].y = 125;

	coords[11].x = 375;
	coords[11].y = 250;

    } else {
	/* random */
	// TODO: implement random coordinates when mesh count is not 8
    }
}

static void init_target_coords()
{
    target_coords.x = 500;
    target_coords.y = 500;
}

void update_coord(u16 node_id, u16 new_x, u16 new_y)
{
    coords[node_id - 1] = (struct simulation_coord_t){ .x = new_x, .y = new_y };
}

u16 distance(struct simulation_coord_t *a, struct simulation_coord_t *b)
{
    // some yolo type conversions here
    double delta_x = b->x - a->x;
    double delta_y = b->y - a->y;
    double d = sqrt(delta_x * delta_x + delta_y * delta_y);
    return (u16)d;
}

bool can_reach_external_target(u16 node_id)
{
    struct simulation_coord_t node_coords = coords[node_id - 1];
    return distance(&node_coords, &target_coords) < SIMULATION_NODE_RANGE;
}

void set_initial_node_ids(struct node_t *node)
{
    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	ARRAY_PUSH(node->known_nodes, (u16)(i + 1));
    }
}

bool can_connect_func(struct node_t *node)
{
    return can_reach_external_target(node->node_id);
}

#ifdef GUI
void pop_and_free(void *arg)
{
    struct queue_t *arrow_queue = (struct queue_t *)arg;
    pthread_mutex_lock(&window_threadpool.cond_var->cond_lock);
    struct arrow_queue_data_t *data = queue_pop(arrow_queue);
    pthread_mutex_unlock(&window_threadpool.cond_var->cond_lock);
    pthread_cond_signal(&window_threadpool.cond_var->cond_variable);
    free(data);
}

void sleep_for_visualization(enum ulsr_internal_packet_type packet_type, u16 from, u16 to,
			     bool is_send)
{
    if (window != NULL) {
	struct window_data_t *ptr = (struct window_data_t *)glfwGetWindowUserPointer(window);
	if (ptr != NULL) {
	    if ((ptr->selected_request_filter == 0 && is_send) ||
		(ptr->selected_request_filter == 1 && !is_send)) {
		return;
	    }
	    switch (ptr->selected_radio_button) {
	    case 0:
		if (packet_type == PACKET_HELLO) {
		    struct arrow_queue_data_t *data = malloc(sizeof(struct arrow_queue_data_t));
		    data->to_node = to;
		    data->from_node = from;
		    data->is_send = is_send;
		    queue_push(ptr->arrow_queue, data);
		    submit_worker_task_timeout(&window_threadpool, pop_and_free, ptr->arrow_queue,
					       1);
		}
		break;
	    case 1:
		if (packet_type == PACKET_DATA) {
		    struct arrow_queue_data_t *data = malloc(sizeof(struct arrow_queue_data_t));
		    data->to_node = to;
		    data->from_node = from;
		    data->is_send = is_send;
		    queue_push(ptr->arrow_queue, data);
		    submit_worker_task_timeout(&window_threadpool, pop_and_free, ptr->arrow_queue,
					       1);
		}
		break;
	    case 2:
		if (packet_type == PACKET_PURGE) {
		    struct arrow_queue_data_t *data = malloc(sizeof(struct arrow_queue_data_t));
		    data->to_node = to;
		    data->from_node = from;
		    data->is_send = is_send;
		    queue_push(ptr->arrow_queue, data);
		    submit_worker_task_timeout(&window_threadpool, pop_and_free, ptr->arrow_queue,
					       1);
		}
		break;
	    case 3:
		if (packet_type == PACKET_ROUTING) {
		    struct arrow_queue_data_t *data = malloc(sizeof(struct arrow_queue_data_t));
		    data->to_node = to;
		    data->from_node = from;
		    data->is_send = is_send;
		    queue_push(ptr->arrow_queue, data);
		    submit_worker_task_timeout(&window_threadpool, pop_and_free, ptr->arrow_queue,
					       1);
		}
		break;
	    case 4:
		if (packet_type == PACKET_ROUTING_DONE) {
		    struct arrow_queue_data_t *data = malloc(sizeof(struct arrow_queue_data_t));
		    data->to_node = to;
		    data->from_node = from;
		    data->is_send = is_send;
		    queue_push(ptr->arrow_queue, data);
		    submit_worker_task_timeout(&window_threadpool, pop_and_free, ptr->arrow_queue,
					       1);
		}
		break;
	    default:
		break;
	    }
	}
    }
}
#endif

u16 send_func(struct ulsr_internal_packet *packet, u16 node_id)
{
#ifdef GUI
    sleep_for_visualization(packet->type, packet->prev_node_id, node_id, true);
#endif
    /*
     * how the simulation mocks whether a packet addressed for this node can't be received due too
     * bad signal
     */
    if (distance(&coords[packet->prev_node_id - 1], &coords[node_id - 1]) > SIMULATION_NODE_RANGE)
	return 0;

    /* how the simulation mocks a packet can't be sent because a node is not active anymore */
    if (nodes[node_id - 1].node_id == NODE_INACTIVE_ID)
	return 0;

    pthread_mutex_lock(&node_locks[node_id - 1].cond_lock);

    /* critical section */
    struct ulsr_internal_packet *new_packet = malloc(sizeof(struct ulsr_internal_packet));
    // TODO: this works for now, but we should implement a better copy function that recursively
    // copy the value of the pointers as well. This implementation only works before we (oh oh)
    // never actually free a packets route or its payload...
    *new_packet = *packet;
    queue_push(&packet_limbo[node_id - 1], new_packet);

    pthread_cond_signal(&node_locks[node_id - 1].cond_variable);
    pthread_mutex_unlock(&node_locks[node_id - 1].cond_lock);

    return packet->payload_len;
};

struct ulsr_internal_packet *recv_func(u16 node_id)
{
    u16 node_idx = node_id - 1;
    pthread_mutex_lock(&node_locks[node_idx].cond_lock);

    /* critical section */
    while (queue_empty(&packet_limbo[node_idx]) && running)
	pthread_cond_wait(&node_locks[node_idx].cond_variable, &node_locks[node_idx].cond_lock);

    struct ulsr_internal_packet *packet;
    if (queue_empty(&packet_limbo[node_idx]))
	packet = NULL;
#ifdef GUI
    else {
	packet = queue_pop(&packet_limbo[node_idx]);
	sleep_for_visualization(packet->type, packet->prev_node_id, node_id, false);
    }
#else
    else
	packet = queue_pop(&packet_limbo[node_idx]);
#endif

    pthread_mutex_unlock(&node_locks[node_idx].cond_lock);
    return packet;
}

bool simulate(void)
{
    running = true;

    /* init the logger mutex */
    logger_init();

    /* mock distance */
    init_coords();
    init_target_coords();

    /* simulated queue of incoming packets */
    init_packet_limbo_queue();

    /* connect, send and recv implementations declared in ulsr/impl.h and defined in uslr/impl.c */
    node_can_connect_func_t node_can_connect_func = can_connect_func;
    node_send_func_t node_send_func = send_func;
    node_recv_func_t node_recv_func = recv_func;

    /* main threadpool */
#ifdef GUI
    /* init the window threadpool */
    init_threadpool(&window_threadpool, MESH_NODE_COUNT + 1, 8);
    start_threadpool(&window_threadpool);
#endif
    init_threadpool(&threadpool, MESH_NODE_COUNT + 1, 8);
    start_threadpool(&threadpool);

    /* init all nodes and make them run on the threadpool */
    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	bool success =
	    init_node(&nodes[i], i + 1, HELLO_POLL_INTERVAL, REMOVE_NEIGHBOR_THRESHOLD,
		      MESH_NODE_COUNT, 8, 8, 8, set_initial_node_ids, node_can_connect_func,
		      node_send_func, node_recv_func, ULSR_DEVICE_PORT_START + i);
	if (!success)
	    exit(1);

	submit_worker_task(&threadpool, run_node_stub, &nodes[i]);
    }

#ifdef GUI
    /* init the window */
    if (!(window = window_create())) {
	goto end_simulation;
    }

    while (!glfwWindowShouldClose(window)) {
	window_update(window);
    }

    window_destroy(window);
#else
    /* run simulation until 'q' is pressed */
    while (running) {
	char c = getc(stdin);
	if (c == 'q')
	    running = false;
	else if (c == 'm') {
	    LOG_INFO("MOVE: node 5");
	    update_coord(5, 500, 500);
	} else if (c == 'd') {
	    LOG_INFO("DESTROY: node 5");
	    destroy_node(&nodes[4]);
	}
    }
#endif

#ifdef GUI
end_simulation:
#endif

    running = false;

    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	pthread_mutex_lock(&node_locks[i].cond_lock);
	pthread_cond_signal(&node_locks[i].cond_variable);
	pthread_mutex_unlock(&node_locks[i].cond_lock);
    }

    LOG_INFO("Stopping MAIN threadpool (this may take some time)");

    // TODO: parallelliser dette
    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	if (nodes[i].node_id == NODE_INACTIVE_ID)
	    continue;
	close_node(&nodes[i]);
	free_node(&nodes[i]);
    }

    threadpool_stop(&threadpool);
    free_threadpool(&threadpool);
#ifdef GUI
    threadpool_stop(&window_threadpool);
    free_threadpool(&window_threadpool);
#endif

    logger_destroy();

    return 0;
}
