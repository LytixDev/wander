#ifndef ROUTING_H
#define ROUTING_H

#include <stdbool.h>
#include <stdlib.h>

/**
 * Struct for data to be sent to the routing thread
 */
struct routing_data_t {
    u16 source_id;
    u16 destination_id;
    u16 total_nodes;
    bool *visited;
    u16 *path;
    u16 path_length;
};

/**
 * Struct for a calculated route
 */
struct route_t {
    u16 source_id;
    u16 destination_id;
    u16 *path;
    u16 path_length;
};

#endif /* ROUTING_H */