#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

/* custom imports */
#include "branch_and_bound.h"

#define TIME_CHECK_INTERVAL 1024

/* It represents the shared state of the search tree exploration */
typedef struct Search {
    Graph graph;
    double** distances;
    double* min_edges;
    int** nearest;
    int* current_route;
    int* visited;
    int* best_route;
    double best_cost;
    unsigned long long explored;
    clock_t start;
    double max_time;
    int timed_out;
} Search;

double calculate_distance(Node a, Node b);
double** build_distance_matrix(Graph graph);
double* calculate_min_edges(Graph graph, double** distances);
int** build_nearest_lists(Graph graph, double** distances);
Solution generate_initial_solution(Graph graph, double** distances);
void branch(Search* search, int depth, double current_cost, double remaining_min);
int has_timed_out(Search* search);
void free_matrix(void** matrix, int size);

Node create_node(char* id, double x, double y);
Graph create_graph(int size);
void add_node(Graph* graph, Node node, int position);
void free_graph(Graph* graph);
void free_solution(Solution* solution);

/*
### Branch and Bound

This function implements the Branch and Bound algorithm.
This algorithm explores the tree of every possible route
in depth, but discards (prunes) every branch whose lower
bound is already worse than the best route found, so it
does not need to check all the n! permutations.

If the whole tree is explored, the returned solution is
the global optimum. If max_time is reached first, it returns
the best route found so far and flags it as not optimal.

Applied in the TSP. Seeking to be applied in the routing of
refrigerated vehicles.

#### Input:
- Graph graph: The graph we're looking to solve.
- double max_time: How much time the algorithm can run in seconds, will end if it reaches it.

#### Output:
- The best solution found inside the graph.

### Example:
```c
branch_and_bound(graph, 120);
```
*/
Solution branch_and_bound(Graph graph, double max_time)
{
    double** distances = build_distance_matrix(graph);                  /* It precalculates the distance between every pair of nodes */
    double* min_edges = calculate_min_edges(graph, distances);          /* the cheapest edge leaving every node (used for the bound) */
    int** nearest = build_nearest_lists(graph, distances);              /* and the neighbors of every node sorted by distance. */

    Solution best = generate_initial_solution(graph, distances);        /* It generates a first greedy solution with Nearest Neighbor */
                                                                        /* to have an upper bound to prune with since the beginning. */

    if (graph.size <= 3) {                                              /* With 3 nodes or less there is only one possible cycle */
        best.is_optimal = 1;                                            /* so the greedy solution is already the optimal one. */
        free(min_edges);
        free_matrix((void**)nearest, graph.size);
        free_matrix((void**)distances, graph.size);
        return best;
    }

    Search search;                                                      /* It prepares the state of the search: */
    search.graph = graph;
    search.distances = distances;
    search.min_edges = min_edges;
    search.nearest = nearest;
    search.current_route = (int *)malloc(sizeof(int) * graph.size);     /* the route that is being built */
    search.visited = (int *)calloc(graph.size, sizeof(int));            /* the nodes already inside that route */
    search.best_route = best.route;                                     /* the best route found (starting with the greedy one) */
    search.best_cost = best.cost;                                       /* and its cost, that works as the upper bound. */
    search.explored = 0;
    search.start = clock();                                             /* Starts the clock */
    search.max_time = max_time;
    search.timed_out = 0;

    double remaining_min = 0.0;                                         /* It sums the cheapest edge of every node, */
    for (int i = 0; i < graph.size; i++) {                              /* so the bound can be updated in O(1) while */
        remaining_min += min_edges[i];                                  /* nodes are added to the route. */
    }

    search.current_route[0] = 0;                                        /* Since the route is a cycle, it fixes the first node */
    search.visited[0] = 1;                                              /* as the start, removing equivalent rotations. */

    branch(&search, 1, 0.0, remaining_min);                             /* It starts exploring the tree from the root. */

    best.route = search.best_route;                                     /* It saves the best route found */
    best.cost = search.best_cost;                                       /* with its cost, */
    best.explored = search.explored;                                    /* how many tree nodes were explored */
    best.is_optimal = !search.timed_out;                                /* and if the whole tree was explored (proven optimal). */

    free(search.current_route);                                         /* It frees all the memory used by the search. */
    free(search.visited);
    free(min_edges);
    free_matrix((void**)nearest, graph.size);
    free_matrix((void**)distances, graph.size);

    return best;                                                        /* It returns the best solution found */
}

/*
This function explores recursively one node of the search tree,
it branches on every unvisited node and prunes every branch
whose lower bound can't improve the best solution.

The lower bound is the cost of the partial route plus the
cheapest edge leaving the last node and every unvisited node,
since each one of them still has to be left exactly once.

#### Input:
- Search* search: A pointer to the shared state of the search.
- int depth: How many nodes are already inside the current route.
- double current_cost: The cost of the current partial route.
- double remaining_min: The sum of the cheapest edges of the last node and the unvisited nodes.

#### Output:
- Nothing, the best route is updated inside the search.
*/
void branch(Search* search, int depth, double current_cost, double remaining_min) {
    if (search->timed_out || has_timed_out(search)) {
        return;
    }

    search->explored++;

    int size = search->graph.size;
    int last = search->current_route[depth - 1];

    if (depth == size) {
        double total_cost = current_cost + search->distances[last][0];

        if (total_cost < search->best_cost) {
            search->best_cost = total_cost;
            memcpy(search->best_route, search->current_route, sizeof(int) * size);
        }

        return;
    }

    for (int k = 0; k < size - 1; k++) {
        int next = search->nearest[last][k];

        if (search->visited[next]) {
            continue;
        }

        double new_cost = current_cost + search->distances[last][next];
        double new_remaining = remaining_min - search->min_edges[last];

        if (new_cost + new_remaining >= search->best_cost) {
            continue;
        }

        search->current_route[depth] = next;
        search->visited[next] = 1;

        branch(search, depth + 1, new_cost, new_remaining);

        search->visited[next] = 0;
    }
}

/*
This function checks if the search has exceeded the max time,
it only reads the clock every TIME_CHECK_INTERVAL explored nodes
because calling clock() is expensive.

#### Input:
- Search* search: A pointer to the shared state of the search.

#### Output:
- 1 if the time is over, 0 if not.
*/
int has_timed_out(Search* search) {
    if (search->explored % TIME_CHECK_INTERVAL != 0) {
        return 0;
    }

    double time_taken = ((double)(clock() - search->start)) / CLOCKS_PER_SEC;

    if (time_taken >= search->max_time) {
        search->timed_out = 1;
    }

    return search->timed_out;
}

/*
This function calculates the distance between two nodes using Pythagoras Theorem

#### Input:
- Node a: The origin node.
- Node b: The destiny node.

#### Output:
- The distance between these two nodes.
*/
double calculate_distance(Node a, Node b) {
    double x_component = b.x - a.x;
    double y_component = b.y - a.y;

    return sqrt(pow(x_component, 2) + pow(y_component, 2));
}

/*
This function precalculates the distance between every pair of nodes,
so the search doesn't need to recalculate them on every branch.

#### Input:
- Graph graph: The graph where all nodes are stored.

#### Output:
- A size x size matrix with the distances.
*/
double** build_distance_matrix(Graph graph) {
    double** distances = (double **)malloc(sizeof(double *) * graph.size);

    for (int i = 0; i < graph.size; i++) {
        distances[i] = (double *)malloc(sizeof(double) * graph.size);

        for (int j = 0; j < graph.size; j++) {
            distances[i][j] = calculate_distance(graph.nodes[i], graph.nodes[j]);
        }
    }

    return distances;
}

/*
This function calculates the cheapest edge leaving every node.

#### Input:
- Graph graph: The graph where all nodes are stored.
- double** distances: The distance matrix of the graph.

#### Output:
- An array with the cheapest edge of every node.
*/
double* calculate_min_edges(Graph graph, double** distances) {
    double* min_edges = (double *)malloc(sizeof(double) * graph.size);

    for (int i = 0; i < graph.size; i++) {
        min_edges[i] = INFINITY;

        for (int j = 0; j < graph.size; j++) {
            if (i != j && distances[i][j] < min_edges[i]) {
                min_edges[i] = distances[i][j];
            }
        }
    }

    return min_edges;
}

/* Distance row used by compare_by_distance, since qsort doesn't accept extra arguments */
static double* sorting_row = NULL;

static int compare_by_distance(const void* a, const void* b) {
    double distance_a = sorting_row[*(const int *)a];
    double distance_b = sorting_row[*(const int *)b];

    return (distance_a > distance_b) - (distance_a < distance_b);
}

/*
This function builds, for every node, the list of the other nodes
sorted from the nearest to the farthest. Exploring the nearest nodes
first finds good routes faster, so more branches get pruned.

#### Input:
- Graph graph: The graph where all nodes are stored.
- double** distances: The distance matrix of the graph.

#### Output:
- A size x (size - 1) matrix with the sorted neighbors of every node.
*/
int** build_nearest_lists(Graph graph, double** distances) {
    int** nearest = (int **)malloc(sizeof(int *) * graph.size);

    for (int i = 0; i < graph.size; i++) {
        nearest[i] = (int *)malloc(sizeof(int) * graph.size);

        int k = 0;
        for (int j = 0; j < graph.size; j++) {
            if (j != i) {
                nearest[i][k++] = j;
            }
        }

        sorting_row = distances[i];
        qsort(nearest[i], k, sizeof(int), compare_by_distance);
    }

    sorting_row = NULL;

    return nearest;
}

/*
This function generates a greedy initial solution using Nearest Neighbor,
always going to the closest node that hasn't been visited yet.

#### Input:
- Graph graph: The graph where all nodes are stored.
- double** distances: The distance matrix of the graph.

#### Output:
- A valid solution with its cost already calculated.
*/
Solution generate_initial_solution(Graph graph, double** distances) {
    Solution solution;

    solution.route = (int *)malloc(sizeof(int) * graph.size);
    solution.cost = 0.0;
    solution.explored = 0;
    solution.is_optimal = 0;

    int* visited = (int *)calloc(graph.size, sizeof(int));

    solution.route[0] = 0;
    visited[0] = 1;

    for (int i = 1; i < graph.size; i++) {
        int last = solution.route[i - 1];
        int closest = -1;

        for (int j = 0; j < graph.size; j++) {
            if (!visited[j] && (closest == -1 || distances[last][j] < distances[last][closest])) {
                closest = j;
            }
        }

        solution.route[i] = closest;
        visited[closest] = 1;
        solution.cost += distances[last][closest];
    }

    solution.cost += distances[solution.route[graph.size - 1]][0];

    free(visited);

    return solution;
}

/*
This function frees a matrix allocated row by row.

#### Input:
- void** matrix: The matrix to free.
- int size: The number of rows of the matrix.

#### Output:
- Nothing.
*/
void free_matrix(void** matrix, int size) {
    for (int i = 0; i < size; i++) {
        free(matrix[i]);
    }

    free(matrix);
}

/*
This functions free the memory used for the route of a solution.

#### Input:
- Solution* solution: A pointer to the solution we want to free.

#### Output:
- Nothing.
*/
void free_solution(Solution* solution) {
    if (solution->route != NULL) {
        free(solution->route);
        solution->route = NULL;
    }
}

/* MAIN UTILS */

/*
This function creates a node with the given parameters.

#### Input:
- char* id: Unique identifier of the node.
- double x: X coordinate of the node.
- double y: Y coordinate of the node.

#### Output:
- A Node struct initialized with the new values.
*/
Node create_node(char* id, double x, double y) {
    Node node;

    strncpy(node.id, id, sizeof(node.id) - 1);
    node.id[sizeof(node.id) - 1] = '\0';
    node.x = x;
    node.y = y;

    return node;
}

/*
This function creates a graph with a given size.

#### Input:
- int size: Number of nodes the graph will contain.

#### Output:
- A Graph struct with allocated memory for nodes.
*/
Graph create_graph(int size) {
    Graph graph;

    graph.size = size;
    graph.nodes = (Node *)malloc(sizeof(Node) * size);

    return graph;
}

/*
This function assigns a node to a specific position in the graph.

#### Input:
- Graph* graph: Pointer to the graph.
- Node node: The node to insert.
- int position: Index where the node will be stored.

#### Output:
- None.
*/
void add_node(Graph* graph, Node node, int position) {
    graph->nodes[position] = node;
}

/*
This function frees the memory allocated for a graph.

#### Input:
- Graph* graph: Pointer to the graph to free.

#### Output:
- None.
*/
void free_graph(Graph* graph) {
    if (graph->nodes != NULL) {
        free(graph->nodes);
        graph->nodes = NULL;
    }
}
