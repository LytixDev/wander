#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "queue.h"

/* Function definitions */

/**
 * A function that will be executed by a worker thread.
 * @param arg The argument to be passed to the function.
 */
typedef void worker_thread_func(void *arg);

/* Structs */

/**
 * Struct used to represent a task.
 * @param func The function to be executed by the worker thread.
 * @param arg The argument to be passed to the function.
 * @param sleep_time The time to sleep before executing the task.
 */
struct task_t {
    worker_thread_func *func;
    void *arg;
    int sleep_time;
};

/**
 * Struct used to represent a condition variable.
 * @param cond_predicate The predicate for the condition variable.
 * @param cond_lock The lock for the condition variable.
 * @param cond_variable The condition variable.
 */
struct condition_t {
    bool cond_predicate;
    pthread_mutex_t cond_lock;
    pthread_cond_t cond_variable;
};

/**
 * Struct used to represent a set of worker threads.
 * @param max_threads The maximum number of threads.
 * @param task_queue The queue of tasks.
 * @param cond_var The condition variable.
 * @param threads The array of threads.
 */
struct threadpool_t {
    int max_threads;
    struct queue_t *task_queue;
    struct condition_t *cond_var;
    pthread_t *threads;
};

/* Methods */

/**
 * Initializes a threadpool.
 * @param threadpool The threadpool to be initialized.
 * @param max_threads The maximum number of threads.
 * @param queue_size The size of the queue.
 */
void init_threadpool(struct threadpool_t *threadpool, int max_threads, int queue_size);

/**
 * Frees the memory allocated for a threadpool.
 * @param threadpool The threadpool to be freed.
 */
void free_threadpool(struct threadpool_t *threadpool);

/**
 * Starts the worker threads.
 * @param threadpool The threadpool to be started.
 */
void start_threadpool(struct threadpool_t *threadpool);

/**
 * Submits a task to the threadpool.
 * @param threadpool The threadpool to submit the task to.
 * @param func The function to be executed by the worker thread.
 * @param arg The argument to be passed to the function.
 * @return True if the task was submitted successfully, false otherwise.
 */
bool submit_worker_task(struct threadpool_t *threadpool, worker_thread_func func, void *arg);

/**
 * Submits a task to the threadpool with a timeout.
 * @param threadpool The threadpool to submit the task to.
 * @param func The function to be executed by the worker thread.
 * @param arg The argument to be passed to the function.
 * @param timeout The timeout in milliseconds.
 * @return True if the task was submitted successfully, false otherwise.
 */
bool submit_worker_task_timeout(struct threadpool_t *threadpool, worker_thread_func func, void *arg,
				int timeout);

/**
 * Stops the threadpool.
 * @param threadpool The threadpool to be stopped.
 */
void threadpool_stop(struct threadpool_t *threadpool);

#endif
