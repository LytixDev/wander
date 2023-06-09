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

#ifndef LOGGER_H
#define LOGGER_H

#include <pthread.h>
#include <stdio.h>

static pthread_mutex_t wander_global_log_mutex;

void logger_init();
void logger_destroy();

#ifdef LOGGING
#define LOG_INFO(...)                                   \
    ({                                                  \
	pthread_mutex_lock(&wander_global_log_mutex);   \
	fprintf(stdout, "\033[0;33m[LOG]: ");           \
	fprintf(stdout, __VA_ARGS__);                   \
	fprintf(stdout, "\033[0m\n");                   \
	pthread_mutex_unlock(&wander_global_log_mutex); \
    })
#else
#define LOG_INFO(...) (void)0
#endif

#ifdef LOGGING
#define LOG_ERR(...)                                    \
    ({                                                  \
	pthread_mutex_lock(&wander_global_log_mutex);   \
	fprintf(stderr, "\033[0;31m[ERR]: ");           \
	fprintf(stderr, __VA_ARGS__);                   \
	fprintf(stderr, "\033[0m\n");                   \
	pthread_mutex_unlock(&wander_global_log_mutex); \
    })
#else
#define LOGG_ERR(...) (void)0
#endif

/*
 * oogah boogah bad coupling smol-brain-dev(tm) dont care :-)
 */

#ifdef LOGGING
#define LOG_NODE_INFO(node_id, ...)                     \
    ({                                                  \
	pthread_mutex_lock(&wander_global_log_mutex);   \
	fprintf(stdout, "\033[0;32m[%d]: ", node_id);   \
	fprintf(stdout, __VA_ARGS__);                   \
	fprintf(stdout, "\033[0m\n");                   \
	pthread_mutex_unlock(&wander_global_log_mutex); \
    })
#else
#define LOGG_NODE_INFO(node_id...) (void)0
#endif

#ifdef LOGGING
#define LOG_NODE_ERR(node_id, ...)                      \
    ({                                                  \
	pthread_mutex_lock(&wander_global_log_mutex);   \
	fprintf(stderr, ":\033[0;31m[%d]: ", node_id);  \
	fprintf(stderr, __VA_ARGS__);                   \
	fprintf(stderr, "\033[0m\n");                   \
	pthread_mutex_unlock(&wander_global_log_mutex); \
    })
#else
#define LOGG_NODE_ERR(node_id, ...) (void)0
#endif

#endif /* LOGGER_H */
