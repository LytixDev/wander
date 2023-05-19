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
 * Struct for a route table.
 */
struct route_table_t {
    struct route_t **entries;
    u16 size;
    u16 cap;
};

/**
 * Initializes a route table.
 * @param table The route table to initialize.
 */
void route_table_init(struct route_table_t *table);

/**
 * Frees a route table.
 * @param table The route table to free.
 */
void route_table_free(struct route_table_t *table);

/**
 * Gets the value of a route table entry.
 * @param table The route table.
 * @param key The key of the entry.
 * @return The value of the entry.
 */
struct route_t *route_table_get(struct route_table_t *table, u16 key);

/**
 * Sets the value of a route table entry.
 * @param table The route table.
 * @param key The key of the entry.
 * @param value The value of the entry.
 */
void route_table_set(struct route_table_t *table, u16 key, struct route_t *value);

/**
 * Removes a route table entry.
 * @param table The route table.
 * @param key The key of the entry.
 */
void route_table_remove(struct route_table_t *table, u16 key);

/**
 * Checks if a route table contains a key.
 * @param table The route table.
 * @param key The key to check.
 * @return True if the route table contains the key, false otherwise.
 */
bool route_table_contains(struct route_table_t *table, u16 key);

#endif /* ROUTE_TABLE_H */
