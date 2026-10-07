from ctypes import *
from pathlib import Path
from typing import Dict, List
import json
import sys
import os

# Models import
from models.problem import Problem, ProblemPtr
from models.solution import Solution, SolutionPtr

# Utils import
from utils.plotter import Plotter, STATUS_NAMES
from utils.utils import Utils


class MainController:
    SIGNS = {"<=": -1, "=": 0, ">=": 1}
    SOLUTION_NAMES = {
        0: "optimal",
        1: "feasible (time or node limit reached, not proven optimal)",
        2: "infeasible (there are no integer points inside the area)",
        3: "unbounded (the objective can grow without limit)",
        4: "not found (time or node limit reached before finding an integer point)",
    }

    @classmethod
    def run(cls, input: str, output: str, max_time: float, max_nodes: int) -> None:
        # Opening the DLL file with the algorithm
        try:
            Utils.log("Opening the C compiled file...")

            output_path: Path = Path(output)
            output_path.mkdir(parents=True, exist_ok=True)
            so_file_path: Path = Path(__file__).parent.parent / "core/target/branch_and_bound.so"
            bnb: CDLL = CDLL(str(so_file_path))
        except Exception as e:
            Utils.error(
                f"There was an error opening the compiled C file, did you already compile it? ({e})"
            )
            sys.exit(1)

        # Setting up the methods of the DLL
        try:
            Utils.log("Setting up all the methods inside of the file...")

            bnb.branch_and_bound.argtypes = [Problem, c_double, c_int]
            bnb.branch_and_bound.restype = Solution

            bnb.create_problem.argtypes = [c_int, c_int, c_int]
            bnb.create_problem.restype = Problem

            bnb.set_objective.argtypes = [ProblemPtr, c_int, c_double]
            bnb.set_objective.restype = None

            bnb.set_constraint.argtypes = [ProblemPtr, c_int, POINTER(c_double), c_int, c_double]
            bnb.set_constraint.restype = None

            bnb.free_problem.argtypes = [ProblemPtr]
            bnb.free_problem.restype = None

            bnb.free_solution.argtypes = [SolutionPtr]
            bnb.free_solution.restype = None

        except Exception as e:
            Utils.error(f"There was an error setting up the methods! ({e})")
            sys.exit(1)

        # Opening the problem file
        try:
            Utils.log("Opening the problem file...")

            with open(Path(input), "r") as input_file:
                data: Dict = json.load(input_file)

            sense: str = data["objective"]["sense"].lower()
            objective: List[float] = [float(c) for c in data["objective"]["coefficients"]]
            n: int = len(objective)
            variables: List[str] = data.get("variables", [f"x{j + 1}" for j in range(n)])
            constraints: List[Dict] = data["constraints"]

            if sense not in ("max", "min"):
                raise ValueError(f"the sense must be 'max' or 'min', not '{sense}'")
            if n == 0:
                raise ValueError("the objective needs at least one coefficient")
            if len(variables) != n:
                raise ValueError(f"there are {len(variables)} variable names but {n} coefficients")
            for i, constraint in enumerate(constraints):
                if len(constraint["coefficients"]) != n:
                    raise ValueError(f"the constraint {i + 1} doesn't have {n} coefficients")
                if constraint["sign"] not in cls.SIGNS:
                    raise ValueError(
                        f"the sign of the constraint {i + 1} must be <=, = or >=, not '{constraint['sign']}'"
                    )
        except Exception as e:
            Utils.error(
                f"There was an error opening the problem, does the file exist and follow the format? ({e})"
            )
            sys.exit(1)

        # Converting the data
        try:
            Utils.log("Converting the data from the problem file...")

            problem: Problem = bnb.create_problem(n, len(constraints), sense == "max")

            for j, coefficient in enumerate(objective):
                bnb.set_objective(pointer(problem), j, coefficient)

            for i, constraint in enumerate(constraints):
                coefficients = (c_double * n)(*[float(c) for c in constraint["coefficients"]])
                bnb.set_constraint(
                    pointer(problem),
                    i,
                    coefficients,
                    cls.SIGNS[constraint["sign"]],
                    float(constraint["rhs"]),
                )
        except Exception as e:
            Utils.error(
                f"There was an error converting the data, check the problem values! ({e})"
            )
            sys.exit(1)

        # Executing the algorithm
        try:
            Utils.log(f"Executing the algorithm (max {max_time} seconds, {max_nodes} nodes)...")
            solution: Solution = bnb.branch_and_bound(problem, max_time, max_nodes)
        except Exception as e:
            Utils.error(f"There was an error executing the algorithm! ({e})")
            sys.exit(1)

        # Reading the solution
        tree: List[Dict] = [
            {
                "id": node.id,
                "parent": node.parent,
                "depth": node.depth,
                "branch_variable": node.branch_variable,
                "branch_sign": node.branch_sign,
                "branch_value": node.branch_value,
                "status": node.status,
                "z": node.z,
                "x": solution.tree_values[node.id * n : (node.id + 1) * n],
            }
            for node in solution.tree[: solution.tree_size]
        ]
        has_solution: bool = solution.status in (0, 1)
        best_x: List[float] = solution.x[:n] if has_solution else None
        best_z: float = solution.z
        best_id: int = cls.find_best_node(tree, best_x)

        if solution.status == 0:
            Utils.log("The whole tree was explored, the solution is optimal!")
        else:
            Utils.warning(f"The solution is {cls.SOLUTION_NAMES[solution.status]}.")

        # Generating the output
        try:
            Utils.log("Generating the solution output...")

            with open(os.path.join(output_path, "solution.txt"), "w") as output_file:
                output_file.write(f"status = {cls.SOLUTION_NAMES[solution.status]}\n")
                if has_solution:
                    output_file.write(f"z = {Plotter.format_number(best_z)}\n")
                    for name, value in zip(variables, best_x):
                        output_file.write(f"{name} = {Plotter.format_number(value)}\n")
                output_file.write(f"explored-nodes = {len(tree)}\n")

                output_file.write("\n--- Branch and Bound Tree ---\n")
                for node in tree:
                    output_file.write(cls.format_node(node, tree, variables) + "\n")

            if not tree:
                Utils.warning("No node was explored, so there is nothing to draw.")
            elif n == 2:
                plane_constraints = [
                    (
                        float(c["coefficients"][0]),
                        float(c["coefficients"][1]),
                        cls.SIGNS[c["sign"]],
                        float(c["rhs"]),
                    )
                    for c in constraints
                ]
                Plotter.draw_tree(tree, variables, best_id, os.path.join(output_path, "tree.png"))
                Plotter.draw_feasible_region(
                    plane_constraints,
                    objective,
                    variables,
                    tree,
                    best_x,
                    best_z,
                    os.path.join(output_path, "feasible_area.png"),
                )
                Plotter.draw_branch_regions(
                    plane_constraints,
                    variables,
                    tree,
                    best_id,
                    os.path.join(output_path, "branches.png"),
                )
            else:
                Plotter.draw_tree(tree, variables, best_id, os.path.join(output_path, "tree.png"))
                Utils.warning(
                    f"The problem has {n} variables, the feasible area can only be drawn with 2."
                )
        except Exception as e:
            Utils.error(f"There was an error generating the output! ({e})")
            sys.exit(1)
        finally:
            bnb.free_solution(pointer(solution))
            bnb.free_problem(pointer(problem))

        Utils.log(f"Done! The results are on {output_path.resolve()}")

    @classmethod
    def find_best_node(cls, tree: List[Dict], best_x: List[float]) -> int:
        if best_x is None:
            return -1
        for node in reversed(tree):
            if node["status"] == 1 and all(
                abs(a - b) < 1e-6 for a, b in zip(node["x"], best_x)
            ):
                return node["id"]
        return -1

    @classmethod
    def format_node(cls, node: Dict, tree: List[Dict], variables: List[str]) -> str:
        indent = "    " * node["depth"]
        origin = (
            "root"
            if node["parent"] == -1
            else f"{Plotter.format_branch(node, variables)} from N{node['parent']}"
        )
        line = f"{indent}N{node['id']} ({origin}): {STATUS_NAMES[node['status']]}"

        if node["status"] not in (3, 4):
            values = ", ".join(
                f"{name} = {Plotter.format_number(value)}"
                for name, value in zip(variables, node["x"])
            )
            line += f" | z = {Plotter.format_number(node['z'])} | {values}"

        return line
