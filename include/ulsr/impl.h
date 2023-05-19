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

#include <pthread.h>
#include <stdbool.h>

#include "lib/common.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"

#define MESH_NODE_COUNT 8
#define HELLO_POLL_INTERVAL 3

#define SIMULATION_NODE_RANGE 1000
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


u16 distance(struct simulation_coord_t *a, struct simulation_coord_t *b);

void set_initial_node_ids(struct node_t *node);

u16 send_func(struct ulsr_internal_packet *packet, u16 node_id);

struct ulsr_internal_packet *recv_func(u16 node_id);

bool simulate(void);

#endif /* IMPL_H */
