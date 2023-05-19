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

#ifndef IMPL_H
#define IMPL_H

/*
 * This is the simulation implementation.
 * In order to implement a new simulation, or implement the protocol in a real world scenario,
 * the functions send_func and recv_func defined in ulsr/node.h need to be implemented.
 * In addition, for any simulation, the simulation() method must be implemented. The simulation
 * method initializes all the simulated nodes and their starting state.
 */

#include <pthread.h>
#include <stdbool.h>

#include "lib/common.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"

/* starting node (device) count */
#define MESH_NODE_COUNT 8
/* how often each node polls every other known node to see if its in range to receive */
#define HELLO_POLL_INTERVAL 3

/* these values are in pixels */
#define SIMULATION_NODE_RANGE 200
#define SIMULATION_WIDTH 800
#define SIMULATION_LENGTH 800


struct await_t {
    pthread_mutex_t cond_lock;
    pthread_cond_t cond_variable;
};

struct simulation_coord_t {
    u16 x;
    u16 y;
};


/* standard euclidian distance for a 2D system */
u16 distance(struct simulation_coord_t *a, struct simulation_coord_t *b);

void set_initial_node_ids(struct node_t *node);

bool can_reach_external_target(u16 node_id);

bool simulate(void);

/* required function implementations */

bool can_connect_func(struct node_t *node);

/* after the packet is sent, a copy is made, and the caller can free the original */
u16 send_func(struct ulsr_internal_packet *packet, u16 node_id);

/* returns one heap allocated packet at a time */
struct ulsr_internal_packet *recv_func(u16 node_id);

#endif /* IMPL_H */
