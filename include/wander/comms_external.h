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

#ifndef COMMS_EXTERNAL_H
#define COMMS_EXTERNAL_H

#include <stdbool.h>

#include "wander/packet.h"
#include "wander/routing.h"


struct external_request_thread_data_t {
    u16 connection;
    struct node_t *node;
};


/* functions */

void handle_external(void *arg);

bool handle_send_external(struct node_t *node, struct wander_internal_packet *packet);


#endif /* COMMS_EXTERNAL_H */
