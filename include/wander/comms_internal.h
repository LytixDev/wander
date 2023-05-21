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

#ifndef COMMS_INTERNAL_H
#define COMMS_INTERNAL_H

#include "wander/node.h"
#include <stdbool.h>

/* functions */

/*
 * main thread run by the node
 * calls the nodes recv function until the nodes running condition is set to false
 */
void main_recv_thread(void *arg);

void hello_poll_thread(void *arg);

u16 find_random_neighbor(struct node_t *node, u16 *path, u16 path_len, u16 *ignore_list,
			 u16 ignore_len);

bool send_bogo(struct wander_internal_packet *packet, struct node_t *node);

bool use_packet_route(struct wander_internal_packet *packet, struct node_t *node);

void propagate_failure(struct wander_internal_packet *packet, struct node_t *node);

#endif /* COMMS_INTERNAL_H */
