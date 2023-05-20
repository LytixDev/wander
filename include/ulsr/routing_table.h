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

#ifndef ROUTE_TABLE_H
#define ROUTE_TABLE_H

#include "lib/common.h"
#include "ulsr/routing.h"
#include <stdbool.h>

#define STD_ROUTE_TABLE_SIZE 32

/**
 * A route table entry.
 */
struct route_table_entry_t {
    struct route_t *route;
    struct route_table_entry_t *prev;
    struct route_table_entry_t *next;
};

/**
 * A route table.
 */
struct route_table_t {
    struct route_table_entry_t *head;
    struct route_table_entry_t *tail;
    u16 size;
};


#endif /* ROUTE_TABLE_H */