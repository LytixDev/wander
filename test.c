#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_NODES 100

typedef struct {
    int edges[MAX_NODES][MAX_NODES];
    int numNodes;
} Graph;

// Function to initialize the graph
void initializeGraph(Graph *graph, int numNodes)
{
    graph->numNodes = numNodes;
    for (int i = 0; i < numNodes; i++) {
	for (int j = 0; j < numNodes; j++) {
	    graph->edges[i][j] = 0;
	}
    }
}

// Function to add an edge between two nodes in the graph
void addEdge(Graph *graph, int node1, int node2)
{
    graph->edges[node1][node2] = 1;
}

// Function to print a path array
void printPath(int *path, int pathLength)
{
    for (int i = 0; i < pathLength; i++) {
	printf("%d ", path[i]);
    }
    printf("\n");
}

// Recursive function to find all paths from source to destination
void findAllPaths(Graph *graph, int source, int destination, bool *visited, int *path,
		  int pathLength)
{
    visited[source] = true;
    path[pathLength++] = source;

    if (source == destination) {
	printPath(path, pathLength);
    } else {
	for (int i = 0; i < graph->numNodes; i++) {
	    if (graph->edges[source][i] == 1 && !visited[i]) {
		findAllPaths(graph, i, destination, visited, path, pathLength);
	    }
	}
    }

    visited[source] = false;
    pathLength--;
}

// Function to find all paths from source to destination in the graph
void findPaths(Graph *graph, int source, int destination)
{
    bool visited[MAX_NODES] = { false };
    int path[MAX_NODES];
    int pathLength = 0;

    findAllPaths(graph, source, destination, visited, path, pathLength);
}

int main()
{
    // Create a graph with 6 nodes (0-5)
    Graph graph;
    initializeGraph(&graph, 6);

    // Add edges between nodes
    addEdge(&graph, 0, 1);
    addEdge(&graph, 0, 2);
    addEdge(&graph, 1, 2);
    addEdge(&graph, 1, 3);
    addEdge(&graph, 2, 3);
    addEdge(&graph, 3, 4);
    addEdge(&graph, 3, 5);
    addEdge(&graph, 4, 5);

    int sourceNode = 0;
    int destinationNode = 5;

    printf("All paths from node %d to node %d:\n", sourceNode, destinationNode);
    findPaths(&graph, sourceNode, destinationNode);

    return 0;
}
