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
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "lib/common.h"
#include "lib/logger.h"
#include "wander/routing.h"
#include "wander/routing_table.h"

bool route_table_empty(struct route_table_t *rt)
{
    if (!rt)
	return true;
    return rt->size == 0;
}

u64 find_longest_time_used(struct route_table_t *rt)
{
    if (route_table_empty(rt))
	return 0;
    struct route_iter_t iter;
    iter_start(&iter, rt);
    u64 max = 0;
    while (!iter_end(&iter)) {
	if (iter.current->route->time_taken > max)
	    max = iter.current->route->time_taken;
	iter_next(&iter);
    }
    return max = max < MAX_WAIT ? max : MAX_WAIT;
}

struct route_t *get_random_route(struct route_table_t *rt)
{
    if (route_table_empty(rt))
	return NULL;
    u64 max = find_longest_time_used(rt);
    int random = rand() % rt->size;
    struct route_iter_t iter;
    iter_start(&iter, rt);
    for (int i = 0; i < random; i++)
	iter_next(&iter);

    struct route_t *found = iter.current->route;
    struct route_t *res = malloc(sizeof(struct route_t));
    max = (((i128)max) - ((i128)found->time_taken)) < 0 ? 0 : max - found->time_taken;
    //    LOG_INFO("Found route from %d to %d that will sleep %ld µs", found->source_id,
    //	     found->destination_id, max);
    init_route(found->source_id, found->destination_id, found->path, found->path_length, max, res);

    return res;
}

struct route_table_entry_t *new_route_entry(struct route_t *route, struct route_table_entry_t *next,
					    struct route_table_entry_t *prev)
{
    struct route_table_entry_t *res = malloc(sizeof(struct route_table_entry_t));
    res->route = route;
    res->next = next;
    res->prev = prev;
    return res;
}

void add_first_pos(struct route_table_t *rt, struct route_t *route)
{
    struct route_table_entry_t *res = new_route_entry(route, NULL, NULL);
    rt->head = res;
    if (!rt->tail)
	rt->tail = res;
    else
	res->next->prev = res;
    rt->size++;
}

void add_last_pos(struct route_table_t *rt, struct route_t *route)
{
    struct route_table_entry_t *res = new_route_entry(route, NULL, rt->tail);
    if (rt->tail)
	rt->tail->next = res;
    else
	rt->head = res;
    rt->tail = res;
    rt->size++;
}

struct route_table_entry_t *remove_entry(struct route_table_t *rt,
					 struct route_table_entry_t *entry)
{
    if (!entry)
	return NULL;
    if (entry->prev)
	entry->prev->next = entry->next;
    else
	rt->head = entry->next;
    if (entry->next)
	entry->next->prev = entry->prev;
    else
	rt->tail = entry->prev;
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
    if (!iter_end(iter))
	iter->current = iter->current->next;
}

void route_entry_free(struct route_table_entry_t *entry)
{
    route_free(entry->route);
    free(entry);
}

void remove_all_entries(struct route_table_t *rt)
{
    struct route_iter_t iter;
    iter_start(&iter, rt);
    while (!iter_end(&iter)) {
	struct route_table_entry_t *entry = iter.current;
	iter_next(&iter);
	route_entry_free(entry);
    }
    rt->head = NULL;
    rt->tail = NULL;
    rt->size = 0;
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
