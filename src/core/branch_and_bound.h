#ifndef BRANCH_AND_BOUND_H
#define BRANCH_AND_BOUND_H

/* Signs of a constraint */
#define SIGN_LESS_EQUAL -1
#define SIGN_EQUAL 0
#define SIGN_GREATER_EQUAL 1

/* States of a node inside the search tree */
#define NODE_BRANCHED 0
#define NODE_INTEGER 1
#define NODE_PRUNED 2
#define NODE_INFEASIBLE 3
#define NODE_UNBOUNDED 4

/* States of the final solution */
#define SOLUTION_OPTIMAL 0
#define SOLUTION_FEASIBLE 1
#define SOLUTION_INFEASIBLE 2
#define SOLUTION_UNBOUNDED 3
#define SOLUTION_NOT_FOUND 4

/* It represents an integer linear program: max/min c·x s.t. A·x (<=, =, >=) b, x >= 0 and integer */
typedef struct Problem {
    int num_variables;
    int num_constraints;
    int maximize;
    double* objective;
    double* coefficients;
    int* signs;
    double* rhs;
} Problem;

/* It represents a subproblem (node) explored inside the search tree */
typedef struct TreeNode {
    int id;
    int parent;
    int depth;
    int branch_variable;
    int branch_sign;
    double branch_value;
    int status;
    double z;
} TreeNode;

/* It represents the final solution and the whole explored tree */
typedef struct Solution {
    double* x;
    double z;
    int status;
    TreeNode* tree;
    double* tree_values;
    int tree_size;
} Solution;

Solution branch_and_bound(Problem problem, double max_time, int max_nodes);

Problem create_problem(int num_variables, int num_constraints, int maximize);
void set_objective(Problem* problem, int variable, double coefficient);
void set_constraint(Problem* problem, int row, double* coefficients, int sign, double rhs);
void free_problem(Problem* problem);
void free_solution(Solution* solution);

#endif
