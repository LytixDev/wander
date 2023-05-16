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

enum ulsr_packet_type {
    ULSR_HTTP,
};

struct ulsr_packet {
    enum ulsr_packet_type type;
    char source_ipv4[16];
    char dest_ipv4[16];
    u16 dest_port;
    u16 payload_len;
    u8 payload[1024];
};

/*
 * packets used for internal communication between devices in the simulation
 */
enum ulsr_internal_packet_type {
    PACKET_DATA,
    PACKET_HELLO,
    PACKET_PURGE,
};

struct ulsr_internal_packet {
    enum ulsr_packet_type pt;
    u16 prev_node_id;
    u32 payload_len;
    void *payload;
};

#endif /* PACKET_H */
