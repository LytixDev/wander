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

#include "ulsr/node.h"
#include "ulsr/packet.h"

static void send_func(struct ulsr_packet *packet)
{
    printf("Sending packet from %s to %s\n", packet->source_ipv4, packet->dest_ipv4);
    free(packet);
}

int main(void)
{
    struct node_t node = { 0 };

    if (init_node(&node, 1, 8, 8, 8, NULL, NULL, NULL, NULL, NULL, 8087) == -1) {
	exit(1);
    }

    if (run_node(&node) == -1) {
	exit(1);
    }

    return 0;
}
