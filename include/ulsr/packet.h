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
    u32 checksum;
    enum ulsr_packet_type type;
    u16 seq_nr;
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
    PACKET_NONE,
    PACKET_ROUTING,
    PACKET_ROUTING_DONE,
};

struct ulsr_internal_packet {
    u32 checksum;
    enum ulsr_internal_packet_type type;
    u16 prev_node_id;
    u16 dest_node_id;
    u32 payload_len;
    void *payload;
};

/* Methods */

/*
 * Creates a new internal packet from an external packet.
 * Allocates the internal packet on the heap.
 */
struct ulsr_internal_packet *ulsr_internal_from_external(struct ulsr_packet *external_packet);

u32 ulsr_checksum(u8 *packet, unsigned long size);

#endif /* PACKET_H */
