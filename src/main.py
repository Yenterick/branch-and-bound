from controllers.main_controller import MainController
import optparse

if __name__ == "__main__":
    parser = optparse.OptionParser()
    parser.add_option(
        "--input",
        "-i",
        help="input problem (.json) to solve with branch and bound",
        default="../examples/integer-program.json",
    )
    parser.add_option(
        "--output", "-o", help="output path to generate the results", default="./target"
    )
    parser.add_option(
        "--max_time",
        "-t",
        type="float",
        help="the max running time in seconds before returning the best solution found",
        default=60,
    )
    parser.add_option(
        "--max_nodes",
        "-m",
        type="int",
        help="the max number of subproblems (tree nodes) to explore",
        default=10000,
    )

    options, _ = parser.parse_args()

    MainController.run(
        options.input,
        options.output,
        options.max_time,
        options.max_nodes,
    )
