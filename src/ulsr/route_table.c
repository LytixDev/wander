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

#include <stdbool.h>
#include <stdlib.h>

#include "lib/arraylist.h"
#include "lib/common.h"
#include "ulsr/route_table.h"
#include "ulsr/routing.h"

void route_table_init(struct route_table_t *table)
{
    table->entries = malloc(sizeof(struct route_t) * STD_ROUTE_TABLE_SIZE);
    table->size = 0;
    table->cap = STD_ROUTE_TABLE_SIZE;
}

void route_table_free(struct route_table_t *table)
{
    free(table->entries);
}

void route_table_add(struct route_table_t *table, struct route_t *route)
{
    u32 tmp = route->destination_id - 1;
    if (tmp >= table->cap) {
	table->cap <<= 1;
	table->entries = realloc(table->entries, sizeof(struct route_t) * table->cap);
    }

    table->entries[tmp] = route;
    table->size++;
}

void route_table_remove(struct route_table_t *table, u16 key)
{
    table->entries[key - 1] = NULL;
    table->size--;
}

bool route_table_contains(struct route_table_t *table, u16 key)
{
    return table->entries[key - 1] != NULL;
}

struct route_t *route_table_get(struct route_table_t *table, u16 key)
{
    return table->entries[key - 1];
}

void route_table_set(struct route_table_t *table, u16 key, struct route_t *value)
{
    table->entries[key - 1] = value;
}
