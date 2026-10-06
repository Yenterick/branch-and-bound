
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

Get a dataset with at least latitude and longitude in grades, if you can't find one, there's already one on the `./examples`

```csv
codigo-de-municipio,municipio,latitud,longitud,metros-sobre-el-nivel-del-mar
76001,cali,3.42158,-76.5205,995
76020,alcala,4.67429,-75.7832,1290
76036,andalucia,4.16701,-76.1662,955
...
```

#### 3. Install Python requirements

The requirements are on `./requirements.txt`:

```txt
numpy==2.5.0
pandas==3.0.3
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

The results (`input_map.png`, `output_map.png` and `best_route.txt`) will be generated on `./src/target` by default.

> [!NOTE]
> Branch and Bound is an exact algorithm, its worst case grows factorially. With ~15 nodes or less it usually proves the optimal route in a few seconds, but with bigger datasets (like the 42 municipalities of the example) it will probably reach `--max_time` first. In that case it returns the best route found so far and `best_route.txt` will show `is-optimal = False`.

---

## Manual

```bash
Usage: main.py [options]

Options:
  -h, --help            show this help message and exit
  -i INPUT, --input=INPUT
                        input dataset to process with branch and bound
  -o OUTPUT, --output=OUTPUT
                        output path to generate the results
  -d ID_FIELD, --id_field=ID_FIELD
                        the name of the field with the node id in the dataset
  -n NAME_FIELD, --name_field=NAME_FIELD
                        the name of the field with the node name in the
                        dataset
  -x X_FIELD, --x_field=X_FIELD
                        the name of the field with the x coord in the dataset
  -y Y_FIELD, --y_field=Y_FIELD
                        the name of the field with the y coord in the dataset
  -t MAX_TIME, --max_time=MAX_TIME
                        the max running time in seconds before returning the
                        best route found
```

---

## About the Project

### About Branch and Bound

Branch and Bound (B&B) is an exact optimization algorithm whose key feature is the ability to find the global optimum without checking every possible solution.
It represents the solution space as a tree, where every level fixes one more decision (in the TSP, the next city of the route), this is the *branching*.
For every node of the tree it calculates a lower bound of the best cost reachable from there, if that bound is already worse than the best solution found, the whole branch is discarded, this is the *bounding* (or pruning).
In this way, unlike metaheuristics like Simulated Annealing, B&B guarantees the optimal solution when it explores the whole tree, at the price of an exponential worst case.

### Algorithm Analysis

```
PROCEDURE Branch-and-Bound(graph, max_time)

Inputs: 
• graph: A graph that contains an array of nodes an its size.
• max_time: The max running in seconds allowed.

Output: A solution that contains an array of nodes sorted in the route order, the cost of the route, the number of explored tree nodes and if it is proven optimal.

1. Precalculate the distance matrix, the cheapest edge leaving every node and the neighbors of every node sorted by distance.
2. Generate an initial solution using Nearest Neighbor and save it as the best solution (upper bound).
3. Fix the first node as the start of the route (it's a cycle, so the rotations are equivalent).
4. Start the clock and explore the tree from the root with BRANCH(route, cost):
    A. If the elapsed time is GREATER THAN max_time, stop the search.
    B. If the route contains every node, close the cycle and, if its cost is LESS THAN the best cost, replace best with it.
    C. If not, for every unvisited node, from the nearest to the farthest:
        C.1. Calculate the lower bound: cost of the route + edge to the node + cheapest edge leaving the node and every remaining node.
        C.2. If the lower bound is GREATER OR EQUAL THAN the best cost, prune the branch.
        C.3. If not, add the node to the route and call BRANCH recursively, then remove it (backtracking).
5. Return the best solution, flagged as optimal if the whole tree was explored.
```

### Complexity Analysis

Let n be the number of nodes in the graph.

- **Precalculation**: O(n² · log n)
- **Initial solution (Nearest Neighbor)**: O(n²)
- **Bound evaluation**: O(1), it is updated incrementally
- **One tree node**: O(n)
- **Total complexity**: O(n · (n - 1)!) in the worst case, but the pruning usually explores a tiny fraction of the tree.
- **Memory**: O(n²) for the distance matrix and O(n) for the recursion.

---

### License

This project is licensed under the [MIT License](./LICENSE).

### Authors

* [TomasSalav](https://github.com/TomasSalav)
* [Yenterick](https://github.com/Yenterick)

### References
- Kernighan, B. W., & Ritchie, D. M. (1988). *The C Programming Language*. Prentice Hall.
- Land, A. H., & Doig, A. G. (1960). An automatic method of solving discrete programming problems. *Econometrica, 28*(3), 497–520.
- Little, J. D. C., Murty, K. G., Sweeney, D. W., & Karel, C. (1963). An algorithm for the traveling salesman problem. *Operations Research, 11*(6), 972–989.
- Cormen, T. H. (2013). *Algorithms Unlocked*. MIT Press.
- Unidad Administrativa Especial de Catastro Distrital, Bogotá D.C. (2026, May 18). *Datos Geográficos De Los Municipios de Valle del Cauca* [Data Set]. Datos.gov.co. https://www.datos.gov.co/Mapas-Nacionales/Datos-Geogr-ficos-De-Los-Municipios-de-Valle-del-C/iryd-wvq5/about_data
