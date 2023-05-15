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

#include "lib/common.h"

/**
 * Enum used to represent the type of a packet.
 * @param PACKET_DATA A packet containing data.
 * @param PACKET_HELLO A packet used to establish a connection.
 * @param PACKET_PURGE A packet used to purge a connection.
 */
enum packet_type {
    PACKET_DATA,
    PACKET_HELLO,
    PACKET_PURGE,
};

/**
 * Struct used to represent a packet.
 * @param source_ip The source IP address of the packet.
 * @param destination_ip The destination IP address of the packet.
 * @param pt The type of packet.
 * @param data The data contained in the packet.
 * @param len The length of the data contained in the packet.
 * This is also the maximum length of the data in an IPv4 packet.
 */
struct packet_h {
    struct sockaddr_in source_ip;
    struct sockaddr_in destination_ip;
    enum packet_type pt;
    void *data;
    u16 len;
};

#endif /* PACKET_H */
