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
#include <netinet/in.h>
#include <string.h>

#include "ulsr/packet.h"
#include "lib/common.h"


void packet_init_hello(struct packet_t *p, struct sockaddr_in source, struct sockaddr_in dest)
{
    u16 len = 32;
    p->header = (struct packet_header_t){ .source_ip = source, .destination_ip = dest,
                                          .pt = PACKET_HELLO, .len = len};
    char *payload = malloc(len);
    strncpy(payload, "Hello", len);
    p->payload = payload;
}

void packet_destroy(struct packet_t *p)
{
    if (p->payload != NULL)
        free(p->payload);
}

/*
* use:
* struct packet_t packet;
* packet_init_hello(p, my_ip, destination_ip)
*
* // send the packet
*
* packet_destroy(p);
*/
