
<p align="center">
  <img src="./docs/banner.png" alt="Branch and Bound">
</p>

<p align="center">
    A C implementation of the Branch and Bound algorithm designed for academic research, combinatorial optimization problems, and experimentation with exact search and pruning techniques.
</p>

<p align="center">
    <a href="#"><img src="https://img.shields.io/badge/Language-C-00599C?style=for-the-badge&logo=c&logoColor=white" alt="Language"></a>
    <a href="#"><img src="https://img.shields.io/badge/Algorithm-Branch%20and%20Bound-orange?style=for-the-badge" alt="Algorithm"></a>
    <a href="#"><img src="https://img.shields.io/badge/Field-Optimization-blue?style=for-the-badge" alt="Field"></a>
    <a href="#"><img src="https://img.shields.io/badge/Research-Academic-purple?style=for-the-badge" alt="Research"></a>
    <a href="#"><img src="https://img.shields.io/badge/Paradigm-Exact%20Search-red?style=for-the-badge" alt="Paradigm"></a>
    <a href="#"><img src="https://img.shields.io/badge/Technique-Pruning-yellow?style=for-the-badge" alt="Technique"></a>
    <a href="#"><img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="License"></a>
</p>

---

## Prerequisites

- gcc (C compiler).
- Make (Makefile).
- Python 3.1x.x.

---

## How to run

#### 1. Compile the Project

In order to run the algorithm you must compile the C files first:
```bash
make
```
or:
```bash
make all
```
It will leave a binary file on the `./src/core/target` folder, in case you want to delete it, you can do:
```bash
make clean
```

#### 2. Prepare your Files

The problem is passed as a `.json` file (the paths and limits are passed as flags). Every variable is considered **integer and >= 0**:

```json
{
    "objective": { "sense": "max", "coefficients": [5, 4] },
    "variables": ["x1", "x2"],
    "constraints": [
        { "coefficients": [6, 4], "sign": "<=", "rhs": 24 },
        { "coefficients": [1, 2], "sign": "<=", "rhs": 6 }
    ]
}
```

- `sense`: `max` or `min`.
- `coefficients`: one coefficient per variable.
- `variables`: optional names, by default `x1, x2, ...`.
- `sign`: `<=`, `=` or `>=`.

To make a variable binary (0 or 1), like in the knapsack problem, add the constraint `x <= 1`. There are some examples on the `./examples` folder: `integer-program.json`, `minimize.json` and `knapsack.json`.

#### 3. Install Python requirements

The requirements are on `./requirements.txt`:

```txt
numpy==2.5.0
matplotlib==3.11.0
```

so you'll just need to run:
```bash
pip install -r requirements.txt
```

#### 4. Run the algorithm

The main file is on `./src/main.py`, so you'll need to navigate inside:
```bash
cd ./src/
```
and then, you can run it with its default values using:
```bash
python main.py
```
or if you want to change a value you can see the flags with:
```bash
python main.py --help
```
or
```bash
python main.py -h
```

The results will be generated on `./src/target` by default:

- `solution.txt`: the optimal solution and every explored subproblem.
- `tree.png`: the Branch and Bound tree.
- `feasible_area.png`: the feasible area with the constraints, the integer points, the branch cuts and the optimum (only with 2 variables).
- `branches.png`: the feasible area of every subproblem (only with 2 variables).

---

## Manual

```bash
Usage: main.py [options]

Options:
  -h, --help            show this help message and exit
  -i INPUT, --input=INPUT
                        input problem (.json) to solve with branch and bound
  -o OUTPUT, --output=OUTPUT
                        output path to generate the results
  -t MAX_TIME, --max_time=MAX_TIME
                        the max running time in seconds before returning the
                        best solution found
  -m MAX_NODES, --max_nodes=MAX_NODES
                        the max number of subproblems (tree nodes) to explore
```

---

## About the Project

### About Branch and Bound

Branch and Bound (B&B) is an exact optimization algorithm, applied here to Integer Linear Programming: optimize a linear objective subject to linear constraints where the variables must be integers.
First it solves the *linear relaxation* (the same problem without the integer requirement) with the Simplex method, its optimum is a corner of the feasible area and gives a bound of the best integer solution.
If a variable of that optimum is fractional, for example x2 = 1.5, it splits (*branches*) the area in two subproblems, x2 <= 1 and x2 >= 2, removing the strip between them where there are no integer points.
Every subproblem whose bound can't improve the best integer solution found is discarded (*pruned*), so, unlike metaheuristics like Simulated Annealing, B&B guarantees the optimal solution when it explores the whole tree.

### Algorithm Analysis

```
PROCEDURE Branch-and-Bound(problem, max_time, max_nodes)

Inputs: 
• problem: An integer linear program (objective, constraints and sense).
• max_time: The max running in seconds allowed.
• max_nodes: The max number of subproblems allowed.

Output: A solution that contains the best integer point, its objective value, its status and the explored tree.

1. Define the best solution (incumbent) as empty, with value -infinity (in max sense).
2. Start the clock and explore the tree from the root (original problem) with BRANCH(bounds):
    A. If the elapsed time is GREATER THAN max_time OR the explored nodes are GREATER THAN max_nodes, stop the search.
    B. Solve the linear relaxation with the bounds using the Two-Phase Simplex.
    C. If it is infeasible, discard the node.
    D. If its value is NOT BETTER THAN the incumbent, prune the node.
    E. If every variable is integer, replace the incumbent with it.
    F. If not, take the first fractional variable x = v and:
        F.1. Call BRANCH adding the bound x <= floor(v).
        F.2. Call BRANCH adding the bound x >= ceil(v).
3. Return the incumbent, flagged as optimal if the whole tree was explored.
```

### Complexity Analysis

Let n be the number of variables and m the number of constraints.

- **Simplex (one relaxation)**: O(m · (n + m)) per pivot, exponential in the worst case but usually polynomial in practice.
- **One tree node**: one Simplex execution.
- **Total complexity**: O(2^d · Simplex), where d is the depth of the tree, exponential in the worst case (Integer Programming is NP-Hard), but the pruning usually explores a tiny fraction of the tree.
- **Memory**: O(m · (n + m)) for the tableau and O(d · n) for the recursion.

---

### License

This project is licensed under the [MIT License](./LICENSE).

### Authors

* [TomasSalav](https://github.com/TomasSalav)
* [Yenterick](https://github.com/Yenterick)

### References
- Kernighan, B. W., & Ritchie, D. M. (1988). *The C Programming Language*. Prentice Hall.
- Land, A. H., & Doig, A. G. (1960). An automatic method of solving discrete programming problems. *Econometrica, 28*(3), 497–520.
- Taha, H. A. (2017). *Operations Research: An Introduction* (10th ed.). Pearson.
- Hillier, F. S., & Lieberman, G. J. (2015). *Introduction to Operations Research* (10th ed.). McGraw-Hill.
- Bland, R. G. (1977). New finite pivoting rules for the simplex method. *Mathematics of Operations Research, 2*(2), 103–107.
