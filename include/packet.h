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

#include <netinet/in.h>

#include "common.h"

enum packet_type {
    PACKET_DATA,
    PACKET_HELLO,
    PACKET_PURGE,
};

struct packet_h {
    struct sockaddr_in source_ip;
    struct sockaddr_in destination_ip;
    enum packet_type pt;
    void *data;
    u16 len;
};

#endif /* PACKET_H */
