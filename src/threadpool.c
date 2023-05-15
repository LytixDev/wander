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

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"
#include "threadpool.h"

void init_threadpool(struct threadpool_t *threadpool, int max_threads, int queue_size)
{
    struct queue_t *queue = (struct queue_t *)(malloc(sizeof(struct queue_t)));
    init_queue(queue, queue_size);
    threadpool->task_queue = queue;

    threadpool->max_threads = max_threads;
    threadpool->threads = (pthread_t *)(malloc(max_threads * sizeof(pthread_t)));

    threadpool->cond_var = (struct condition_t *)(malloc(sizeof(struct condition_t)));
    threadpool->cond_var->cond_predicate = true;
    pthread_mutex_init(&threadpool->cond_var->cond_lock, NULL);
    pthread_cond_init(&threadpool->cond_var->cond_variable, NULL);
}

void threadpool_stop(struct threadpool_t *workers)
{
    pthread_mutex_lock(&workers->cond_var->cond_lock);
    workers->cond_var->cond_predicate = false;
    pthread_cond_broadcast(&workers->cond_var->cond_variable);
    pthread_mutex_unlock(&workers->cond_var->cond_lock);
    for (int i = 0; i < workers->max_threads; i++)
	pthread_join(*(workers->threads + i), NULL);
}

void free_threadpool(struct threadpool_t *workers)
{
    free_queue(workers->task_queue);
    free(workers->task_queue);

    free(workers->threads);

    pthread_mutex_destroy(&workers->cond_var->cond_lock);
    pthread_cond_destroy(&workers->cond_var->cond_variable);

    free(workers->cond_var);
}

static void *start_worker_thread(void *arg)
{
    struct threadpool_t *data = (struct threadpool_t *)arg;
    while (true) {
	struct task_t *item;
	pthread_mutex_lock(&data->cond_var->cond_lock);
	while (queue_empty(data->task_queue) && data->cond_var->cond_predicate)
	    pthread_cond_wait(&data->cond_var->cond_variable, &data->cond_var->cond_lock);
	item = (struct task_t *)queue_pop(data->task_queue);
	pthread_cond_signal(&data->cond_var->cond_variable);
	pthread_mutex_unlock(&data->cond_var->cond_lock);

	if (item != NULL && data->cond_var->cond_predicate) {
	    usleep(item->sleep_time);
	    item->func(item->arg);
	    free(item);
	} else {
	    pthread_exit(NULL);
	}
    }
}

void start_threadpool(struct threadpool_t *workers)
{
    for (int i = 0; i < workers->max_threads; i++)
	pthread_create(workers->threads + i, NULL, start_worker_thread, workers);
}

static bool submit_task(struct threadpool_t *workers, struct task_t *task)
{
    pthread_mutex_lock(&workers->cond_var->cond_lock);
    bool ret = queue_push(workers->task_queue, (void *)task);
    while (!ret) {
	pthread_cond_wait(&workers->cond_var->cond_variable, &workers->cond_var->cond_lock);
	ret = queue_push(workers->task_queue, (void *)task);
    }
    pthread_mutex_unlock(&workers->cond_var->cond_lock);
    pthread_cond_signal(&workers->cond_var->cond_variable);
    return ret;
}

bool submit_worker_task(struct threadpool_t *workers, worker_thread_func func, void *arg)
{
    struct task_t *task = (struct task_t *)(malloc(sizeof(struct task_t)));
    task->func = func;
    task->arg = arg;
    task->sleep_time = 0;
    return submit_task(workers, task);
}

bool submit_worker_task_timeout(struct threadpool_t *workers, worker_thread_func func, void *arg,
				int timeout)
{
    struct task_t *task = (struct task_t *)(malloc(sizeof(struct task_t)));
    task->func = func;
    task->arg = arg;
    task->sleep_time = timeout;
    return submit_task(workers, task);
}