#ifndef BRANCH_AND_BOUND_H
#define BRANCH_AND_BOUND_H

/* On the intended application, represents a city */
typedef struct Node {
    char id[20];
    double x, y;
} Node;

/* On the intended application, represents a set of cities */
typedef struct Graph {
    Node* nodes;
    unsigned int size;
} Graph;

/* It represents a possible solution */
typedef struct Solution {
    int* route;
    double cost;
    unsigned long long explored;
    int is_optimal;
} Solution;

Solution branch_and_bound(Graph graph, double max_time);

Node create_node(char* id, double x, double y);
Graph create_graph(int size);
void add_node(Graph* graph, Node node, int position);
void free_graph(Graph* graph);
void free_solution(Solution* solution);

#endif
