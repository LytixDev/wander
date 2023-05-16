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

#include "ulsr/ulsr.h"
#include "ulsr/node.h"
#include "ulsr/packet.h"

static u16 send_func(struct ulsr_internal_packet *packet, u16 node_id)
{
}

int main(void)
{
    #define MESH_NODE_COUNT 8
    struct node_t nodes[MESH_NODE_COUNT];
    struct ulsr_internal_packetl current_packets[MESH_NODE_COUNT];

    /* init nodes */
    for (int i = 0; i < MESH_NODE_COUNT; i++) {
        int rc = init_node(&nodes[i], i + 1, 8, 8, 8, NULL, send_func, NULL, NULL, NULL, ULSR_DEVICE_PORT_START + i);
        if (rc == -1)
            exit(1);
    }




    //struct node_t node = { 0 };

    //if (init_node(&node, 1, 8, 8, 8, NULL, send_func, NULL, NULL, NULL, 8087) == -1) {
    //    exit(1);
    //}

    //if (run_node(&node) == -1) {
    //    exit(1);
    //}

    //free_node(&node);

    return 0;
}
