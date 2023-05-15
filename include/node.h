#ifndef NODE_H
#define NODE_H

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct neighbor_t {
    struct sockaddr_in addr;
    int cost;
};

struct node_t {
    struct sockaddr_in addr;
    struct neighbor_t *neighbors;
    struct sockaddr_in *routing_table;
};

#endif /* NODE_H */