from ctypes import *
from pathlib import Path
from typing import List
import matplotlib.pyplot as plt
import pandas as pd
import sys
import os

# Models import
from models.graph import Graph, GraphPtr
from models.node import Node
from models.solution import Solution, SolutionPtr

# Utils import
from utils.utils import Utils


class MainController:
    EARTH_DEGREE_TO_KM = 111.32

    @classmethod
    def run(
        cls,
        input: str,
        output: str,
        id_field: str,
        name_field: str,
        x_field: str,
        y_field: str,
        max_time: float,
    ) -> None:
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

            bnb.branch_and_bound.argtypes = [Graph, c_double]
            bnb.branch_and_bound.restype = Solution

            bnb.create_node.argtypes = [c_char_p, c_double, c_double]
            bnb.create_node.restype = Node

            bnb.create_graph.argtypes = [c_int]
            bnb.create_graph.restype = Graph

            bnb.add_node.argtypes = [GraphPtr, Node, c_int]
            bnb.add_node.restype = None

            bnb.free_graph.argtypes = [GraphPtr]
            bnb.free_graph.restype = None

            bnb.free_solution.argtypes = [SolutionPtr]
            bnb.free_solution.restype = None

        except Exception as e:
            Utils.error(f"There was an error setting up the methods! ({e})")
            sys.exit(1)

        # Opening the dataset
        try:
            Utils.log("Opening the dataset...")

            input_path: Path = Path(input)
            dataframe: pd.DataFrame = pd.read_csv(input_path)

            if dataframe.empty:
                raise ValueError("the dataset has no rows")

            plt.figure()
            plt.scatter(dataframe[x_field], dataframe[y_field])
            plt.xlabel("X Axis")
            plt.ylabel("Y Axis")
            plt.title("Input Cartesian Map")
            plt.savefig(os.path.join(output_path, "input_map.png"))
            plt.close()
        except Exception as e:
            Utils.error(
                f"There was an error opening the dataset, does the file really exist? ({e})"
            )
            sys.exit(1)

        # Converting the data
        try:
            Utils.log("Converting the data from the dataset...")

            dataframe = dataframe.reset_index(drop=True)
            graph: Graph = bnb.create_graph(len(dataframe.index))

            for i in dataframe.index:
                id = str(dataframe[id_field][i]).encode("utf-8")
                node = bnb.create_node(
                    id, float(dataframe[x_field][i]), float(dataframe[y_field][i])
                )
                bnb.add_node(pointer(graph), node, i)

        except Exception as e:
            Utils.error(
                f"There was an error converting the data, check the convertion parameters! ({e})"
            )
            sys.exit(1)

        # Executing the algorithm
        try:
            Utils.log(f"Executing the algorithm (max {max_time} seconds)...")
            solution: Solution = bnb.branch_and_bound(graph, max_time)
        except Exception as e:
            Utils.error(f"There was an error executing the algorithm! ({e})")
            sys.exit(1)

        if solution.is_optimal:
            Utils.log("The whole tree was explored, the route is optimal!")
        else:
            Utils.warning(
                "The time ran out, the route is the best found but it's not proven optimal."
            )

        # Generating the output
        try:
            Utils.log("Generating the solution output...")

            best_route: List[int] = solution.route[: graph.size]
            best_route.append(best_route[0])  # We need to close the cycle

            formatted_route: List[str] = [
                f"{dataframe.at[i, name_field]} ({dataframe.at[i, id_field]})"
                for i in best_route
            ]

            plt.figure()
            plt.plot(
                dataframe.loc[best_route, x_field], dataframe.loc[best_route, y_field], "-o"
            )
            plt.xlabel("X Axis")
            plt.ylabel("Y Axis")
            plt.title("Output Cartesian Map")
            plt.savefig(os.path.join(output_path, "output_map.png"))
            plt.close()

            with open(os.path.join(output_path, "best_route.txt"), "w") as output_file:
                output_file.write("\n\t V \n".join(formatted_route))
                output_file.write("\n")
                output_file.write(
                    f"\ntotal-cost = {solution.cost * cls.EARTH_DEGREE_TO_KM} km\n"
                )  # We need to convert grades to km
                output_file.write(f"explored-nodes = {solution.explored}\n")
                output_file.write(f"is-optimal = {bool(solution.is_optimal)}\n")
        except Exception as e:
            Utils.error(f"There was an error generating the output! ({e})")
            sys.exit(1)
        finally:
            bnb.free_solution(pointer(solution))
            bnb.free_graph(pointer(graph))

        Utils.log(f"Done! The results are on {output_path.resolve()}")
