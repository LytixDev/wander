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
#include "ulsr/packet.h"

#define MESH_NODE_COUNT 8


struct await_t {
    pthread_mutex_t cond_lock;
    pthread_cond_t cond_variable;
};


u16 send_func(struct ulsr_internal_packet *packet, u16 node_id);

struct ulsr_internal_packet *recv_func(u16 node_id);

bool simulate(void);

#endif /* IMPL_H */
