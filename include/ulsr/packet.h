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

#ifndef PACKET_H
#define PACKET_H

#include "lib/common.h"


/*
 * packets used for internal communication between devices in the simulation
 */

enum packet_type {
    PACKET_DATA,
    PACKET_HELLO,
    PACKET_PURGE,
};

struct packet_header_t {
    enum packet_type pt;
    u16 source_node_id;
    u16 destination_node_id;
    u16 len;
};

struct packet_t {
    struct packet_header_t header;
    void *payload;
};

#endif /* PACKET_H */
