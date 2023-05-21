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

#include "lib/common.h"
#include "ulsr/routing_table.h"
#include "ulsr/routing.h"

bool route_table_empty(struct route_table_t *rt)
{
    if (!rt) return true;
    return rt->size == 0;
}

bool route_is_old(struct route_table_entry_t *rt)
{
    return rt->use_count > MAX_AGE;
}

struct route_t *get_random_route(struct route_table_t *rt)
{
    if (route_table_empty(rt)) return NULL;
    int random = rand() % rt->size;
    struct route_iter_t iter;
    iter_start(&iter, rt);
    for (int i = 0; i < random; i++) iter_next(&iter);
    iter.current->use_count++;
    if (route_is_old(iter.current))
    return remove_entry(rt, iter.current)->route;
    return iter.current->route;
}

struct route_table_entry_t *new_route_entry(struct route_t *route, struct route_table_entry_t *next,
					    struct route_table_entry_t *prev)
{
    struct route_table_entry_t *res = malloc(sizeof(struct route_table_entry_t));
    res->route = route;
    res->next = next;
    res->prev = prev;
    res->use_count = 0;
    return res;
}

void add_first_pos(struct route_table_t *rt, struct route_t *route)
{
    struct route_table_entry_t *res = new_route_entry(route, NULL, NULL);
    rt->head = res;
    if (!rt->tail) rt->tail = res;
    else res->next->prev = res;
    rt->size++;
}

void add_last_pos(struct route_table_t *rt, struct route_t *route)
{
    struct route_table_entry_t *res = new_route_entry(route, NULL, rt->tail);
    if (rt->tail) rt->tail->next = res;
    else rt->head = res;
    rt->tail = res;
    rt->size++;
}

struct route_table_entry_t *remove_entry(struct route_table_t *rt, struct route_table_entry_t *entry)
{
    if (entry->prev) entry->prev->next = entry->next;
    else rt->head = entry->next;
    if (entry->next) entry->next->prev = entry->prev;
    else rt->tail = entry->prev;
    rt->size--;
    entry->next = NULL;
    entry->prev = NULL;
    return entry;
}

void iter_start(struct route_iter_t *iter, struct route_table_t *rt)
{
    iter->current = rt->head;
}

int iter_end(struct route_iter_t *iter)
{
    return !iter->current;
}

void iter_next(struct route_iter_t *iter)
{
    if(!iter_end(iter))
        iter->current = iter->current->next;
}

void route_entry_free(struct route_table_entry_t *entry)
{
    free(entry->route->path);
    free(entry->route);
    free(entry);
}

void route_table_free(struct route_table_t *rt)
{
    struct route_iter_t iter;
    iter_start(&iter, rt);
    while (!iter_end(&iter)) {
        struct route_table_entry_t *entry = iter.current;
        iter_next(&iter);
        route_entry_free(entry);
    }
    free(rt);
}