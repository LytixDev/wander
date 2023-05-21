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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lib/common.h"
#include "wander/packet.h"
#include "wander/routing.h"
#include "wander/wander.h"

char *uslr_internal_type_to_str[PACKET_TYPE_COUNT] = {
    "DATA", "HELLO", "PURGE", "ROUTING", "ROUTING_DONE", "NONE",
};

struct wander_packet *wander_create_response(struct wander_packet *packet, u8 *response, u16 seq_nr)
{
    struct wander_packet *response_packet = malloc(sizeof(struct wander_packet));
    strncpy(response_packet->source_ipv4, packet->dest_ipv4, 16);
    strncpy(response_packet->dest_ipv4, packet->source_ipv4, 16);
    response_packet->dest_port = WANDER_DEFAULT_PORT;
    response_packet->payload_len = strlen((char *)response);
    strncpy((char *)response_packet->payload, (char *)response, response_packet->payload_len);
    response_packet->type = WANDER_RESPONSE;
    response_packet->seq_nr = seq_nr;
    return response_packet;
}

struct wander_packet *wander_create_failure(struct wander_packet *packet_that_failed)
{
    struct wander_packet *response_packet = malloc(sizeof(struct wander_packet));
    strncpy(response_packet->source_ipv4, packet_that_failed->dest_ipv4, 16);
    strncpy(response_packet->dest_ipv4, packet_that_failed->source_ipv4, 16);
    response_packet->dest_port = WANDER_DEFAULT_PORT;
    response_packet->type = WANDER_INTERNAL_FAILURE;
    response_packet->seq_nr = 0;
    response_packet->checksum = 0;
    return response_packet;
}

struct wander_internal_packet *wander_internal_from_external(struct wander_packet *external)
{
    struct wander_internal_packet *packet = malloc(sizeof(struct wander_internal_packet));
    packet->payload_len = sizeof(struct wander_packet);
    packet->payload = external;
    packet->prev_node_id = 0;
    packet->dest_node_id = 0;
    packet->type = PACKET_DATA;
    packet->is_response = false;
    return packet;
}

struct wander_internal_packet *wander_internal_create_hello(u16 from, u16 to)
{
    struct wander_internal_packet *packet = malloc(sizeof(struct wander_internal_packet));
    packet->type = PACKET_HELLO;
    packet->payload_len = 0;
    packet->payload = NULL;
    packet->prev_node_id = from;
    packet->dest_node_id = to;
    return packet;
}

void wander_packet_free(struct wander_packet *packet)
{
    free(packet);
}

void wander_internal_packet_free(struct wander_internal_packet *packet)
{
    if (packet->payload != NULL)
        free(packet->payload);
    free(packet);
}


// TODO: use checksums everywhere
u32 wander_checksum(u8 *packet, unsigned long size)
{
    u32 checksum = 0;

    /* i starts at 4 because we don't want to include the checksum in the checksum calculation */
    for (unsigned long i = 4; i < size; i++)
	checksum += packet[i];

    return ~checksum;
}
