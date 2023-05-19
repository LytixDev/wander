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
#include <stdbool.h>

enum ulsr_packet_type {
    ULSR_HTTP,
    ULSR_RESPONSE,
    ULSR_FAILURE,
};

struct ulsr_packet {
    u32 checksum;
    enum ulsr_packet_type type;
    u16 seq_nr;
    char source_ipv4[16];
    char dest_ipv4[16];
    u16 dest_port;
    u16 payload_len;
    u8 payload[UINT16_MAX];
};

/*
 * packets used for internal communication between devices in the simulation
 */
enum ulsr_internal_packet_type {
    PACKET_DATA, // data packets final destination will always be an external entity
    PACKET_HELLO,
    PACKET_PURGE,
    PACKET_ROUTING,
    PACKET_ROUTING_DONE,
    PACKET_NONE,
};

static char *uslr_internal_type_to_str[] = {
    "DATA", "HELLO", "PURGE", "ROUTING", "ROUTING_DONE", "NONE",
};

struct packet_route_t {
    u16 *path;
    u16 len;
    u16 step;
};

struct ulsr_internal_packet {
    u32 checksum;
    enum ulsr_internal_packet_type type;
    u16 prev_node_id;
    u16 dest_node_id;
    u32 payload_len;
    void *payload;
    struct packet_route_t *route;
    bool is_response;
};

/* functions */

struct ulsr_packet *ulsr_create_response(struct ulsr_packet *packet, u8 *response, u16 seq_nr);

/*
 * Creates a new internal packet from an external packet.
 * Allocates the internal packet on the heap.
 */
struct ulsr_internal_packet *ulsr_internal_from_external(struct ulsr_packet *external_packet);

struct ulsr_internal_packet *ulsr_internal_create_hello(u16 from, u16 to);

u32 ulsr_checksum(u8 *packet, unsigned long size);

#endif /* PACKET_H */
