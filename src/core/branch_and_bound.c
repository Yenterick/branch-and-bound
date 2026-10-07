#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

/* custom imports */
#include "branch_and_bound.h"

#define EPSILON 1e-9
#define FEASIBILITY_TOLERANCE 1e-7
#define INTEGER_TOLERANCE 1e-6

#define LP_OPTIMAL 0
#define LP_INFEASIBLE 1
#define LP_UNBOUNDED 2

/* It represents the shared state of the search tree exploration */
typedef struct Search {
    Problem problem;
    double sense;
    double* best_x;
    double best_z;
    int has_incumbent;
    int unbounded;
    TreeNode* tree;
    double* tree_values;
    int tree_size;
    int tree_capacity;
    int max_nodes;
    clock_t start;
    double max_time;
    int stopped;
} Search;

void branch(Search* search, double* lower, double* upper, int parent, int depth, int branch_variable, int branch_sign, double branch_value);
int solve_relaxation(Problem* problem, double sense, double* lower, double* upper, double* x, double* z);
int run_simplex(double** tableau, int* basis, int rows, int columns, int allowed_columns);
void pivot(double** tableau, int rows, int columns, int pivot_row, int pivot_column);
int find_fractional_variable(double* x, int num_variables);
int add_tree_node(Search* search, int parent, int depth, int branch_variable, int branch_sign, double branch_value);
int has_to_stop(Search* search);

Problem create_problem(int num_variables, int num_constraints, int maximize);
void set_objective(Problem* problem, int variable, double coefficient);
void set_constraint(Problem* problem, int row, double* coefficients, int sign, double rhs);
void free_problem(Problem* problem);
void free_solution(Solution* solution);

/*
### Branch and Bound

This function implements the Branch and Bound algorithm
for Integer Linear Programming.

It solves the linear relaxation of the problem (ignoring the
integer requirement) with the Simplex method, if the answer has
a fractional variable x = v, it splits (branches) the feasible area
in two subproblems: one with x <= floor(v) and one with x >= ceil(v),
removing the area between them where no integer point exists.

The relaxation value of every subproblem is a bound of the best
integer solution inside its area, so if it is not better than the
best integer solution found (incumbent), the branch is discarded (pruned).

#### Input:
- Problem problem: The integer linear program we're looking to solve.
- double max_time: How much time the algorithm can run in seconds, will end if it reaches it.
- int max_nodes: How many subproblems the algorithm can explore, will end if it reaches it.

#### Output:
- The best integer solution found and the whole explored tree.

### Example:
```c
branch_and_bound(problem, 60, 10000);
```
*/
Solution branch_and_bound(Problem problem, double max_time, int max_nodes)
{
    int n = problem.num_variables;

    Search search;                                                      /* It prepares the state of the search: */
    search.problem = problem;
    search.sense = problem.maximize ? 1.0 : -1.0;                       /* the sense, so minimizing c·x is maximizing -c·x */
    search.best_x = (double *)calloc(n, sizeof(double));                /* the best integer solution found (incumbent) */
    search.best_z = -INFINITY;                                          /* and its value, that works as the bound to prune with. */
    search.has_incumbent = 0;
    search.unbounded = 0;
    search.tree_capacity = 64;                                          /* It reserves memory to remember every explored node */
    search.tree_size = 0;                                               /* so the tree can be drawn later. */
    search.tree = (TreeNode *)malloc(sizeof(TreeNode) * search.tree_capacity);
    search.tree_values = (double *)malloc(sizeof(double) * search.tree_capacity * n);
    search.max_nodes = max_nodes;
    search.start = clock();                                             /* Starts the clock */
    search.max_time = max_time;
    search.stopped = 0;

    double* lower = (double *)calloc(n, sizeof(double));                /* Every variable starts on the range [0, infinity) */
    double* upper = (double *)malloc(sizeof(double) * n);               /* since the branches will only tighten these bounds. */
    for (int j = 0; j < n; j++) {
        upper[j] = INFINITY;
    }

    branch(&search, lower, upper, -1, 0, -1, 0, 0.0);                   /* It starts exploring the tree from the root (original problem). */

    Solution solution;                                                  /* It builds the final solution: */
    solution.x = search.best_x;                                         /* the best integer point */
    solution.z = search.sense * search.best_z;                          /* with its value on the original sense */
    solution.tree = search.tree;                                        /* and the explored tree. */
    solution.tree_values = search.tree_values;
    solution.tree_size = search.tree_size;

    if (search.unbounded) {                                             /* If a relaxation had no limit, the problem is unbounded. */
        solution.status = SOLUTION_UNBOUNDED;
    } else if (search.has_incumbent) {                                  /* If an integer point was found, it is optimal only */
        solution.status = search.stopped                                /* if the whole tree was explored. */
            ? SOLUTION_FEASIBLE
            : SOLUTION_OPTIMAL;
    } else {                                                            /* If not, there are no integer points in the area */
        solution.status = search.stopped                                /* or the time ran out before finding one. */
            ? SOLUTION_NOT_FOUND
            : SOLUTION_INFEASIBLE;
    }

    free(lower);                                                        /* It frees the memory used by the search. */
    free(upper);

    return solution;                                                    /* It returns the best solution found */
}

/*
This function explores recursively one subproblem of the search tree:
it solves its relaxation and decides if it must be pruned, if it is
an integer solution or if it has to be split in two new subproblems.

#### Input:
- Search* search: A pointer to the shared state of the search.
- double* lower: The lower bound of every variable on this subproblem.
- double* upper: The upper bound of every variable on this subproblem.
- int parent: The id of the parent node (-1 for the root).
- int depth: The depth of the node inside the tree.
- int branch_variable: The variable restricted to create this node (-1 for the root).
- int branch_sign: The sign of that restriction (<= or >=).
- double branch_value: The value of that restriction.

#### Output:
- Nothing, the incumbent and the tree are updated inside the search.
*/
void branch(Search* search, double* lower, double* upper, int parent, int depth, int branch_variable, int branch_sign, double branch_value) {
    if (has_to_stop(search)) {
        return;
    }

    int n = search->problem.num_variables;
    int id = add_tree_node(search, parent, depth, branch_variable, branch_sign, branch_value);
    double* x = &search->tree_values[id * n];
    double z;

    int lp_status = solve_relaxation(&search->problem, search->sense, lower, upper, x, &z);

    if (lp_status == LP_INFEASIBLE) {
        search->tree[id].status = NODE_INFEASIBLE;
        return;
    }

    if (lp_status == LP_UNBOUNDED) {
        search->tree[id].status = NODE_UNBOUNDED;
        search->unbounded = 1;
        return;
    }

    search->tree[id].z = search->sense * z;

    if (search->has_incumbent && z <= search->best_z + FEASIBILITY_TOLERANCE) {
        search->tree[id].status = NODE_PRUNED;
        return;
    }

    int fractional = find_fractional_variable(x, n);

    if (fractional == -1) {
        search->tree[id].status = NODE_INTEGER;
        search->best_z = z;
        search->has_incumbent = 1;

        for (int j = 0; j < n; j++) {
            search->best_x[j] = round(x[j]);
        }

        return;
    }

    search->tree[id].status = NODE_BRANCHED;

    double value = x[fractional];
    double* child_bounds = (double *)malloc(sizeof(double) * n);

    memcpy(child_bounds, upper, sizeof(double) * n);
    child_bounds[fractional] = floor(value);
    branch(search, lower, child_bounds, id, depth + 1, fractional, SIGN_LESS_EQUAL, floor(value));

    memcpy(child_bounds, lower, sizeof(double) * n);
    child_bounds[fractional] = ceil(value);
    branch(search, child_bounds, upper, id, depth + 1, fractional, SIGN_GREATER_EQUAL, ceil(value));

    free(child_bounds);
}

/*
This function solves the linear relaxation of a subproblem using the
Two-Phase Simplex method. The bounds of the variables are added as
extra constraints (x <= upper and x >= lower).

Phase 1 finds a feasible corner of the area minimizing the artificial
variables, phase 2 moves between corners improving the objective.

#### Input:
- Problem* problem: A pointer to the problem.
- double sense: 1 to maximize or -1 to minimize.
- double* lower: The lower bound of every variable.
- double* upper: The upper bound of every variable.
- double* x: Where the values of the variables will be written.
- double* z: Where the value of the objective (in max sense) will be written.

#### Output:
- LP_OPTIMAL, LP_INFEASIBLE or LP_UNBOUNDED.
*/
int solve_relaxation(Problem* problem, double sense, double* lower, double* upper, double* x, double* z) {
    int n = problem->num_variables;
    int rows = problem->num_constraints;

    for (int j = 0; j < n; j++) {
        if (upper[j] < lower[j] - EPSILON) {
            return LP_INFEASIBLE;
        }
        if (!isinf(upper[j])) rows++;
        if (lower[j] > 0) rows++;
    }

    double* row_coefficients = (double *)calloc(rows * n, sizeof(double));
    int* row_signs = (int *)malloc(sizeof(int) * rows);
    double* row_rhs = (double *)malloc(sizeof(double) * rows);

    int r = 0;
    for (int i = 0; i < problem->num_constraints; i++, r++) {
        memcpy(&row_coefficients[r * n], &problem->coefficients[i * n], sizeof(double) * n);
        row_signs[r] = problem->signs[i];
        row_rhs[r] = problem->rhs[i];
    }
    for (int j = 0; j < n; j++) {
        if (!isinf(upper[j])) {
            row_coefficients[r * n + j] = 1.0;
            row_signs[r] = SIGN_LESS_EQUAL;
            row_rhs[r++] = upper[j];
        }
        if (lower[j] > 0) {
            row_coefficients[r * n + j] = 1.0;
            row_signs[r] = SIGN_GREATER_EQUAL;
            row_rhs[r++] = lower[j];
        }
    }

    int num_slacks = 0;
    int num_artificials = 0;

    for (int i = 0; i < rows; i++) {
        if (row_rhs[i] < 0) {
            for (int j = 0; j < n; j++) {
                row_coefficients[i * n + j] *= -1;
            }
            row_rhs[i] *= -1;
            row_signs[i] *= -1;
        }
        if (row_signs[i] != SIGN_EQUAL) num_slacks++;
        if (row_signs[i] != SIGN_LESS_EQUAL) num_artificials++;
    }

    int artificial_start = n + num_slacks;
    int columns = artificial_start + num_artificials + 1;
    int rhs_column = columns - 1;

    double** tableau = (double **)malloc(sizeof(double *) * (rows + 1));
    for (int i = 0; i <= rows; i++) {
        tableau[i] = (double *)calloc(columns, sizeof(double));
    }
    int* basis = (int *)malloc(sizeof(int) * rows);

    int slack = n;
    int artificial = artificial_start;

    for (int i = 0; i < rows; i++) {
        memcpy(tableau[i], &row_coefficients[i * n], sizeof(double) * n);
        tableau[i][rhs_column] = row_rhs[i];

        if (row_signs[i] == SIGN_LESS_EQUAL) {
            tableau[i][slack] = 1.0;
            basis[i] = slack++;
        } else {
            if (row_signs[i] == SIGN_GREATER_EQUAL) {
                tableau[i][slack++] = -1.0;
            }
            tableau[i][artificial] = 1.0;
            basis[i] = artificial++;
        }
    }

    free(row_coefficients);
    free(row_signs);
    free(row_rhs);

    int status = LP_OPTIMAL;

    if (num_artificials > 0) {
        for (int j = artificial_start; j < rhs_column; j++) {
            tableau[rows][j] = 1.0;
        }
        for (int i = 0; i < rows; i++) {
            if (basis[i] >= artificial_start) {
                for (int j = 0; j < columns; j++) {
                    tableau[rows][j] -= tableau[i][j];
                }
            }
        }

        run_simplex(tableau, basis, rows, columns, rhs_column);

        if (tableau[rows][rhs_column] < -FEASIBILITY_TOLERANCE) {
            status = LP_INFEASIBLE;
        } else {
            for (int i = 0; i < rows; i++) {
                if (basis[i] < artificial_start) continue;

                for (int j = 0; j < artificial_start; j++) {
                    if (fabs(tableau[i][j]) > EPSILON) {
                        pivot(tableau, rows, columns, i, j);
                        basis[i] = j;
                        break;
                    }
                }
            }
        }
    }

    if (status == LP_OPTIMAL) {
        memset(tableau[rows], 0, sizeof(double) * columns);
        for (int j = 0; j < n; j++) {
            tableau[rows][j] = -sense * problem->objective[j];
        }
        for (int i = 0; i < rows; i++) {
            double factor = tableau[rows][basis[i]];

            if (factor != 0.0) {
                for (int j = 0; j < columns; j++) {
                    tableau[rows][j] -= factor * tableau[i][j];
                }
            }
        }

        status = run_simplex(tableau, basis, rows, columns, artificial_start);
    }

    if (status == LP_OPTIMAL) {
        memset(x, 0, sizeof(double) * n);
        for (int i = 0; i < rows; i++) {
            if (basis[i] < n) {
                x[basis[i]] = tableau[i][rhs_column];
            }
        }
        *z = tableau[rows][rhs_column];
    } else {
        memset(x, 0, sizeof(double) * n);
    }

    for (int i = 0; i <= rows; i++) {
        free(tableau[i]);
    }
    free(tableau);
    free(basis);

    return status;
}

/*
This function runs the Simplex iterations over a tableau until no
column can improve the objective, using Bland's rule (always the
lowest index) to avoid cycling.

#### Input:
- double** tableau: The tableau, the last row is the objective.
- int* basis: The basic variable of every row.
- int rows: The number of constraint rows.
- int columns: The number of columns (including the rhs).
- int allowed_columns: Only the columns before this index can enter the basis.

#### Output:
- LP_OPTIMAL or LP_UNBOUNDED.
*/
int run_simplex(double** tableau, int* basis, int rows, int columns, int allowed_columns) {
    int rhs_column = columns - 1;

    while (1) {
        int entering = -1;

        for (int j = 0; j < allowed_columns; j++) {
            if (tableau[rows][j] < -EPSILON) {
                entering = j;
                break;
            }
        }

        if (entering == -1) {
            return LP_OPTIMAL;
        }

        int leaving = -1;
        double best_ratio = INFINITY;

        for (int i = 0; i < rows; i++) {
            if (tableau[i][entering] <= EPSILON) continue;

            double ratio = tableau[i][rhs_column] / tableau[i][entering];

            if (ratio < best_ratio - EPSILON ||
                (fabs(ratio - best_ratio) <= EPSILON && basis[i] < basis[leaving])) {
                best_ratio = ratio;
                leaving = i;
            }
        }

        if (leaving == -1) {
            return LP_UNBOUNDED;
        }

        pivot(tableau, rows, columns, leaving, entering);
        basis[leaving] = entering;
    }
}

/*
This function makes a pivot on the tableau, turning the pivot column
into a unit column with the 1 on the pivot row.

#### Input:
- double** tableau: The tableau.
- int rows: The number of constraint rows.
- int columns: The number of columns (including the rhs).
- int pivot_row: The row of the leaving variable.
- int pivot_column: The column of the entering variable.

#### Output:
- Nothing.
*/
void pivot(double** tableau, int rows, int columns, int pivot_row, int pivot_column) {
    double pivot_value = tableau[pivot_row][pivot_column];

    for (int j = 0; j < columns; j++) {
        tableau[pivot_row][j] /= pivot_value;
    }

    for (int i = 0; i <= rows; i++) {
        if (i == pivot_row) continue;

        double factor = tableau[i][pivot_column];

        if (factor == 0.0) continue;

        for (int j = 0; j < columns; j++) {
            tableau[i][j] -= factor * tableau[pivot_row][j];
        }
    }
}

/*
This function looks for the first variable that is not integer.

#### Input:
- double* x: The values of the variables.
- int num_variables: The number of variables.

#### Output:
- The index of the first fractional variable, -1 if all of them are integer.
*/
int find_fractional_variable(double* x, int num_variables) {
    for (int j = 0; j < num_variables; j++) {
        if (fabs(x[j] - round(x[j])) > INTEGER_TOLERANCE) {
            return j;
        }
    }

    return -1;
}

/*
This function saves a new node on the tree, growing its memory if needed.

#### Input:
- Search* search: A pointer to the shared state of the search.
- int parent: The id of the parent node.
- int depth: The depth of the node.
- int branch_variable: The variable restricted to create this node.
- int branch_sign: The sign of that restriction.
- double branch_value: The value of that restriction.

#### Output:
- The id of the new node.
*/
int add_tree_node(Search* search, int parent, int depth, int branch_variable, int branch_sign, double branch_value) {
    int n = search->problem.num_variables;

    if (search->tree_size == search->tree_capacity) {
        search->tree_capacity *= 2;
        search->tree = (TreeNode *)realloc(search->tree, sizeof(TreeNode) * search->tree_capacity);
        search->tree_values = (double *)realloc(search->tree_values, sizeof(double) * search->tree_capacity * n);
    }

    int id = search->tree_size++;

    TreeNode node;
    node.id = id;
    node.parent = parent;
    node.depth = depth;
    node.branch_variable = branch_variable;
    node.branch_sign = branch_sign;
    node.branch_value = branch_value;
    node.status = NODE_INFEASIBLE;
    node.z = 0.0;

    search->tree[id] = node;

    return id;
}

/*
This function checks if the search has exceeded the max time
or the max number of explored nodes.

#### Input:
- Search* search: A pointer to the shared state of the search.

#### Output:
- 1 if the search has to stop, 0 if not.
*/
int has_to_stop(Search* search) {
    if (search->stopped) {
        return 1;
    }

    double time_taken = ((double)(clock() - search->start)) / CLOCKS_PER_SEC;

    if (time_taken >= search->max_time || search->tree_size >= search->max_nodes) {
        search->stopped = 1;
    }

    return search->stopped;
}

/*
This functions free the memory used by a solution and its tree.

#### Input:
- Solution* solution: A pointer to the solution we want to free.

#### Output:
- Nothing.
*/
void free_solution(Solution* solution) {
    free(solution->x);
    free(solution->tree);
    free(solution->tree_values);

    solution->x = NULL;
    solution->tree = NULL;
    solution->tree_values = NULL;
}

/* MAIN UTILS */

/*
This function creates an empty problem with the given size.

#### Input:
- int num_variables: Number of variables of the problem.
- int num_constraints: Number of constraints of the problem.
- int maximize: 1 to maximize the objective, 0 to minimize it.

#### Output:
- A Problem struct with allocated memory for the objective and the constraints.
*/
Problem create_problem(int num_variables, int num_constraints, int maximize) {
    Problem problem;

    problem.num_variables = num_variables;
    problem.num_constraints = num_constraints;
    problem.maximize = maximize;
    problem.objective = (double *)calloc(num_variables, sizeof(double));
    problem.coefficients = (double *)calloc(num_variables * num_constraints, sizeof(double));
    problem.signs = (int *)calloc(num_constraints, sizeof(int));
    problem.rhs = (double *)calloc(num_constraints, sizeof(double));

    return problem;
}

/*
This function assigns the coefficient of a variable in the objective.

#### Input:
- Problem* problem: Pointer to the problem.
- int variable: Index of the variable.
- double coefficient: Its coefficient in the objective.

#### Output:
- None.
*/
void set_objective(Problem* problem, int variable, double coefficient) {
    problem->objective[variable] = coefficient;
}

/*
This function assigns a constraint to a specific row of the problem.

#### Input:
- Problem* problem: Pointer to the problem.
- int row: Index where the constraint will be stored.
- double* coefficients: The coefficient of every variable.
- int sign: SIGN_LESS_EQUAL, SIGN_EQUAL or SIGN_GREATER_EQUAL.
- double rhs: The right hand side of the constraint.

#### Output:
- None.
*/
void set_constraint(Problem* problem, int row, double* coefficients, int sign, double rhs) {
    int n = problem->num_variables;

    memcpy(&problem->coefficients[row * n], coefficients, sizeof(double) * n);
    problem->signs[row] = sign;
    problem->rhs[row] = rhs;
}

/*
This function frees the memory allocated for a problem.

#### Input:
- Problem* problem: Pointer to the problem to free.

#### Output:
- None.
*/
void free_problem(Problem* problem) {
    free(problem->objective);
    free(problem->coefficients);
    free(problem->signs);
    free(problem->rhs);

    problem->objective = NULL;
    problem->coefficients = NULL;
    problem->signs = NULL;
    problem->rhs = NULL;
}
