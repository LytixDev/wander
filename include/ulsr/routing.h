#ifndef ROUTING_H
#define ROUTING_H

#include <stdbool.h>
#include <stdlib.h>

#include "lib/arraylist.h"
#include "lib/common.h"

/**
 * Struct for an arraylist of routes
 */
ARRAY_T(route_array_t, struct route_t);

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
    u32 time_taken;
};

/* Methods */

/**
 * Initializes the routing data struct
 * 
 * @param source_id The source node ID
 * @param destination_id The destination node ID
 * @param total_nodes The total number of nodes in the network
 * @param visited The visited array
 * @param path The path array
 * @param path_length The length of the path
 * @param routing_data The routing data struct to initialize
 */
void init_routing_data(u16 source_id, u16 destination_id, u16 total_nodes, bool *visited, u16 *path,
                       u16 path_length, struct routing_data_t *routing_data);

/**
 * Initializes the route struct
 * 
 * @param source_id The source node ID
 * @param destination_id The destination node ID
 * @param path The path array
 * @param path_length The length of the path
 * @param route The route struct to initialize
 */
void init_route(u16 source_id, u16 destination_id, u16 *path, u16 path_length, struct route_t *route);

/**
 * Frees the routing data struct
 * 
 * @param routing_data The routing data struct to free
 */
void free_routing_data(struct routing_data_t *routing_data);

/**
 * Frees the route struct
 * 
 * @param route The route struct to free
 */
void free_route(struct route_t *route);

/**
 * Method that is called by each node to find all routes to the destination node
 * This function then sends the routes to the next node in the path
 * 
 * @param curr The current node
 * @param destination_id The destination node ID
 * @param total_nodes The total number of nodes in the network
 * @param visited The visited array
 * @param path The path array
 * @param path_length The length of the path
 */
void find_all_routes_send(struct node_t *curr, u16 destination_id, u16 total_nodes, bool *visited,
                          u16 *path, u16 path_length);

/**
 * Finds all routes from the current node to the destination node
 * 
 * @param start The starting node
 * @param destination_id The destination node ID
 * @param total_nodes The total number of nodes in the network
 */
void find_all_routes(struct node_t *start, u16 destination_id, u16 total_nodes);

/**
 * Finds the longest time taken by any route
 * 
 * @param routes The arraylist of routes
 * @return The longest time taken by any route
 */
u32 find_longest_time_taken(struct route_array_t *routes);

#endif /* ROUTING_H */