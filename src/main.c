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

#include <stdio.h>

#include "lib/common.h"
#include "lib/lambda.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"
#include "ulsr/ulsr.h"

#define MESH_NODE_COUNT 2

int main(void)
{
    struct node_t nodes[MESH_NODE_COUNT];
    struct ulsr_internal_packet packet_limbo[MESH_NODE_COUNT] = { 0 };

    struct node_t node_one = { 0 };

    node_send_func_t node_send_func =
	LAMBDA(u16, (struct ulsr_internal_packet * packet, u16 node_id), {
	    packet_limbo[node_id] = *packet;
	    return packet->payload_len;
	});

    node_rec_func_t node_rec_func = LAMBDA(struct ulsr_internal_packet *, (u16 node_id), {
	struct ulsr_internal_packet *packet = malloc(sizeof(struct ulsr_internal_packet));
	*packet = packet_limbo[node_id];
	packet_limbo[node_id] = (struct ulsr_internal_packet){ 0 };
	packet_limbo[node_id].pt = PACKET_NONE;
	return packet;
    });

    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	int rc = init_node(&nodes[i], i + 1, 8, 8, 8, NULL, node_send_func, node_rec_func, NULL,
			   NULL, ULSR_DEVICE_PORT_START + i);
	if (rc == -1)
	    exit(1);
	rc = run_node(&nodes[i]);
	if (rc == -1)
	    exit(1);
    }

    for (int i = 0; i < MESH_NODE_COUNT; i++) {
	free_node(&nodes[i]);
    }

    return 0;
}
