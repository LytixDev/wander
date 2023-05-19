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

#include <stdlib.h>
#include <string.h>

#include "lib/common.h"
#include "ulsr/packet.h"

struct ulsr_internal_packet *ulsr_internal_from_external(struct ulsr_packet *external)
{
    struct ulsr_internal_packet *packet = malloc(sizeof(struct ulsr_internal_packet));
    packet->payload_len = sizeof(struct ulsr_packet);
    packet->payload = external;
    packet->prev_node_id = 0;
    packet->dest_node_id = 0;
    packet->type = PACKET_DATA;
    return packet;
}

struct ulsr_internal_packet *ulsr_internal_create_hello(u16 from, u16 to)
{
    struct ulsr_internal_packet *packet = malloc(sizeof(struct ulsr_internal_packet));
    packet->type = PACKET_HELLO;
    packet->payload_len = 0;
    packet->payload = NULL;
    packet->prev_node_id = from;
    packet->dest_node_id = to;
    return packet;
}


u32 ulsr_checksum(u8 *packet, unsigned long size)
{
    u32 checksum = 0;

    /* i starts at 4 because we don't want to include the checksum in the checksum calculation */
    for (unsigned long i = 4; i < size; i++)
	checksum += packet[i];

    return ~checksum;
}
