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

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stdlib.h>

/* Structs */

/**
 * Struct used to represent a queue.
 * @param items The items in the queue.
 * @param start The start of the queue.
 * @param end The end of the queue.
 * @param size The size of the queue.
 * @param max The max size of the queue.
 */
struct queue_t {
    void **items;
    int start;
    int end;
    int size;
    int max;
};


/* Methods */

/**
 * Function used to initialize a queue.
 * @param queue queue to initialize.
 * @param size size of the queue.
 */
void init_queue(struct queue_t *queue, int size);

/**
 * Function used to check if a queue is empty.
 * @param queue queue to check.
 * @return true if the queue is empty, false otherwise.
 */
bool queue_empty(struct queue_t *queue);

/**
 * Function used to check if a queue is full.
 * @param queue queue to check.
 * @return true if the queue is full, false otherwise.
 */
bool queue_full(struct queue_t *queue);

/**
 * Function used to push an item to a queue.
 * @param queue queue to push to.
 * @param item item to push.
 * @return true if the item was pushed, false otherwise.
 */
bool queue_push(struct queue_t *queue, void *item);

/**
 * Function used to pop an item from a queue.
 * @param queue queue to pop from.
 * @return the item popped from the queue.
 */
void *queue_pop(struct queue_t *queue);

/**
 * Function used to free a queue.
 * @param queue queue to free.
 */
void free_queue(struct queue_t *queue);

#endif
