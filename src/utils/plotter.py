from itertools import combinations
from typing import Dict, List, Tuple
import math
import matplotlib.pyplot as plt

# A constraint on the plane: (a1, a2, sign, b) means a1·x1 + a2·x2 (sign) b
Constraint = Tuple[float, float, int, float]

SIGN_LESS_EQUAL = -1
SIGN_EQUAL = 0
SIGN_GREATER_EQUAL = 1

SIGN_SYMBOLS = {SIGN_LESS_EQUAL: "≤", SIGN_EQUAL: "=", SIGN_GREATER_EQUAL: "≥"}

STATUS_NAMES = {
    0: "branched",
    1: "integer",
    2: "pruned",
    3: "infeasible",
    4: "unbounded",
}

STATUS_COLORS = {
    0: "#4C72B0",
    1: "#55A868",
    2: "#8C8C8C",
    3: "#C44E52",
    4: "#DD8452",
}

TOLERANCE = 1e-7


class Plotter:
    MAX_TREE_LABELS = 40
    MAX_REGION_PLOTS = 16
    MAX_LATTICE_POINTS = 2500

    @classmethod
    def format_number(cls, value: float) -> str:
        if abs(value - round(value)) < 1e-6:
            return str(int(round(value)))
        return f"{value:.2f}".rstrip("0").rstrip(".")

    @classmethod
    def format_branch(cls, node: Dict, variables: List[str]) -> str:
        return (
            f"{variables[node['branch_variable']]} "
            f"{SIGN_SYMBOLS[node['branch_sign']]} "
            f"{cls.format_number(node['branch_value'])}"
        )

    @classmethod
    def draw_tree(
        cls, tree: List[Dict], variables: List[str], best_id: int, path: str
    ) -> None:
        # The ids are assigned in DFS preorder, so a parent always comes before its children
        children: Dict[int, List[int]] = {node["id"]: [] for node in tree}
        for node in tree:
            if node["parent"] != -1:
                children[node["parent"]].append(node["id"])

        positions: Dict[int, float] = {}
        next_leaf = 0
        for node in tree:
            if not children[node["id"]]:
                positions[node["id"]] = next_leaf
                next_leaf += 1
        for node in reversed(tree):
            if children[node["id"]]:
                child_positions = [positions[c] for c in children[node["id"]]]
                positions[node["id"]] = sum(child_positions) / len(child_positions)

        max_depth = max(node["depth"] for node in tree)
        show_labels = len(tree) <= cls.MAX_TREE_LABELS

        width = max(6, next_leaf * (2.6 if show_labels else 0.3))
        height = max(4, (max_depth + 1) * (1.9 if show_labels else 0.4))
        fig, ax = plt.subplots(figsize=(min(width, 60), min(height, 60)))

        for node in tree:
            if node["parent"] == -1:
                continue
            x0, y0 = positions[node["parent"]], -tree[node["parent"]]["depth"]
            x1, y1 = positions[node["id"]], -node["depth"]
            ax.plot([x0, x1], [y0, y1], color="#444444", linewidth=1, zorder=1)
            if show_labels:
                ax.text(
                    (x0 + x1) / 2,
                    (y0 + y1) / 2,
                    cls.format_branch(node, variables),
                    ha="center",
                    va="center",
                    fontsize=8,
                    bbox=dict(boxstyle="round,pad=0.2", fc="white", ec="none"),
                    zorder=2,
                )

        for node in tree:
            x, y = positions[node["id"]], -node["depth"]
            color = STATUS_COLORS[node["status"]]
            is_best = node["id"] == best_id

            if not show_labels:
                ax.scatter(x, y, color=color, s=60 if is_best else 12, zorder=3)
                continue

            lines = [f"N{node['id']}"]
            if node["status"] in (3, 4):
                lines.append(STATUS_NAMES[node["status"]])
            else:
                lines.append(f"z = {cls.format_number(node['z'])}")
                lines.append(
                    ", ".join(
                        f"{name} = {cls.format_number(value)}"
                        for name, value in zip(variables, node["x"])
                    )
                    if len(variables) <= 4
                    else f"{len(variables)} variables"
                )
                lines.append(STATUS_NAMES[node["status"]] + (" (best)" if is_best else ""))

            ax.text(
                x,
                y,
                "\n".join(lines),
                ha="center",
                va="center",
                fontsize=8,
                color="white",
                bbox=dict(
                    boxstyle="round,pad=0.4",
                    fc=color,
                    ec="#E1A100" if is_best else color,
                    linewidth=3 if is_best else 1,
                ),
                zorder=3,
            )

        handles = [
            plt.Line2D([], [], marker="s", linestyle="", color=STATUS_COLORS[s], label=n)
            for s, n in STATUS_NAMES.items()
        ]
        ax.legend(handles=handles, loc="upper right", fontsize=8)
        ax.set_xlim(-1, max(next_leaf, 1))
        ax.set_ylim(-max_depth - 0.7, 0.7)
        ax.set_title("Branch and Bound Tree")
        ax.axis("off")
        fig.tight_layout()
        fig.savefig(path, dpi=150)
        plt.close(fig)

    @classmethod
    def node_constraints(cls, tree: List[Dict], node_id: int) -> List[Constraint]:
        constraints: List[Constraint] = []
        node = tree[node_id]
        while node["parent"] != -1:
            a = [0.0, 0.0]
            a[node["branch_variable"]] = 1.0
            constraints.append((a[0], a[1], node["branch_sign"], node["branch_value"]))
            node = tree[node["parent"]]
        return constraints

    @classmethod
    def is_inside(cls, point: Tuple[float, float], constraints: List[Constraint]) -> bool:
        for a1, a2, sign, b in constraints:
            value = a1 * point[0] + a2 * point[1]
            scale = max(1.0, abs(b))
            if sign == SIGN_LESS_EQUAL and value > b + TOLERANCE * scale:
                return False
            if sign == SIGN_GREATER_EQUAL and value < b - TOLERANCE * scale:
                return False
            if sign == SIGN_EQUAL and abs(value - b) > TOLERANCE * scale:
                return False
        return True

    @classmethod
    def region_vertices(
        cls, constraints: List[Constraint], limit: float
    ) -> List[Tuple[float, float]]:
        # The area is clipped to the box [0, limit] x [0, limit] so unbounded areas can be drawn
        box: List[Constraint] = [
            (1, 0, SIGN_GREATER_EQUAL, 0),
            (0, 1, SIGN_GREATER_EQUAL, 0),
            (1, 0, SIGN_LESS_EQUAL, limit),
            (0, 1, SIGN_LESS_EQUAL, limit),
        ]
        all_constraints = constraints + box

        vertices: List[Tuple[float, float]] = []
        for (a1, a2, _, b), (c1, c2, _, d) in combinations(all_constraints, 2):
            determinant = a1 * c2 - a2 * c1
            if abs(determinant) < 1e-12:
                continue
            point = ((b * c2 - a2 * d) / determinant, (a1 * d - b * c1) / determinant)
            if cls.is_inside(point, all_constraints) and not any(
                math.dist(point, v) < 1e-9 for v in vertices
            ):
                vertices.append(point)

        if len(vertices) < 3:
            return vertices

        cx = sum(v[0] for v in vertices) / len(vertices)
        cy = sum(v[1] for v in vertices) / len(vertices)
        return sorted(vertices, key=lambda v: math.atan2(v[1] - cy, v[0] - cx))

    @classmethod
    def plot_limit(cls, constraints: List[Constraint], tree: List[Dict]) -> float:
        candidates = [1.0]
        for a1, a2, _, b in constraints:
            for a in (a1, a2):
                if abs(a) > 1e-12 and b / a > 0:
                    candidates.append(b / a)
        for node in tree:
            if node["status"] not in (3, 4):
                candidates.extend(node["x"])
        return math.ceil(max(candidates) * 1.15) + 1

    @classmethod
    def format_constraint(cls, constraint: Constraint, variables: List[str]) -> str:
        a1, a2, sign, b = constraint
        left = ""
        for a, name in ((a1, variables[0]), (a2, variables[1])):
            if abs(a) < 1e-12:
                continue
            term = ("" if abs(a) == 1 else cls.format_number(abs(a))) + name
            if not left:
                left = ("-" if a < 0 else "") + term
            else:
                left += f" {'-' if a < 0 else '+'} {term}"
        return f"{left or '0'} {SIGN_SYMBOLS[sign]} {cls.format_number(b)}"

    @classmethod
    def draw_line(cls, ax, constraint: Constraint, limit: float, **kwargs) -> None:
        a1, a2, _, b = constraint
        if abs(a2) > 1e-12:
            xs = [0, limit]
            ys = [(b - a1 * x) / a2 for x in xs]
        elif abs(a1) > 1e-12:
            xs = [b / a1, b / a1]
            ys = [0, limit]
        else:
            return
        ax.plot(xs, ys, **kwargs)

    @classmethod
    def draw_feasible_region(
        cls,
        constraints: List[Constraint],
        objective: List[float],
        variables: List[str],
        tree: List[Dict],
        best_x: List[float],
        best_z: float,
        path: str,
    ) -> None:
        limit = cls.plot_limit(constraints, tree)
        fig, ax = plt.subplots(figsize=(8, 8))

        vertices = cls.region_vertices(constraints, limit)
        if len(vertices) >= 3:
            ax.fill(
                [v[0] for v in vertices],
                [v[1] for v in vertices],
                color="#4C72B0",
                alpha=0.25,
                label="LP feasible area",
            )

        line_colors = plt.cm.tab10.colors
        for i, constraint in enumerate(constraints):
            cls.draw_line(
                ax,
                constraint,
                limit,
                color=line_colors[i % len(line_colors)],
                linewidth=1.5,
                label=cls.format_constraint(constraint, variables),
            )

        if (limit + 1) ** 2 <= cls.MAX_LATTICE_POINTS:
            lattice = [
                (i, j)
                for i in range(int(limit) + 1)
                for j in range(int(limit) + 1)
                if cls.is_inside((i, j), constraints)
            ]
            ax.scatter(
                [p[0] for p in lattice],
                [p[1] for p in lattice],
                color="black",
                s=12,
                zorder=3,
                label="Integer points",
            )

        cuts = set()
        for node in tree[1:]:
            cut = (node["branch_variable"], node["branch_value"])
            if cut in cuts:
                continue
            cuts.add(cut)
            a = [0.0, 0.0]
            a[cut[0]] = 1.0
            cls.draw_line(
                ax,
                (a[0], a[1], SIGN_EQUAL, cut[1]),
                limit,
                color="#8C8C8C",
                linestyle="--",
                linewidth=1,
            )
        if cuts:
            ax.plot([], [], color="#8C8C8C", linestyle="--", label="Branch cuts")

        root = tree[0]
        if root["status"] not in (3, 4):
            ax.scatter(*root["x"], color="#C44E52", marker="X", s=120, zorder=4,
                       label=f"LP optimum ({', '.join(cls.format_number(v) for v in root['x'])})")

        if best_x is not None:
            if any(abs(c) > 1e-12 for c in objective):
                cls.draw_line(
                    ax,
                    (objective[0], objective[1], SIGN_EQUAL, best_z),
                    limit,
                    color="#55A868",
                    linestyle=":",
                    linewidth=2,
                    label=f"z = {cls.format_number(best_z)}",
                )
            ax.scatter(*best_x, color="#E1A100", marker="*", s=350, zorder=5,
                       edgecolors="black",
                       label=f"Integer optimum ({', '.join(cls.format_number(v) for v in best_x)})")

        ax.set_xlim(0, limit)
        ax.set_ylim(0, limit)
        ax.set_xlabel(variables[0])
        ax.set_ylabel(variables[1])
        ax.set_title("Feasible Area")
        ax.grid(alpha=0.3)
        ax.legend(fontsize=8, loc="upper right")
        fig.tight_layout()
        fig.savefig(path, dpi=150)
        plt.close(fig)

    @classmethod
    def draw_branch_regions(
        cls,
        constraints: List[Constraint],
        variables: List[str],
        tree: List[Dict],
        best_id: int,
        path: str,
    ) -> None:
        limit = cls.plot_limit(constraints, tree)
        nodes = tree[: cls.MAX_REGION_PLOTS]
        columns = min(4, len(nodes))
        rows = math.ceil(len(nodes) / columns)

        fig, axes = plt.subplots(
            rows, columns, figsize=(4 * columns, 4 * rows), squeeze=False
        )
        root_vertices = cls.region_vertices(constraints, limit)

        for ax in axes.flat[len(nodes):]:
            ax.axis("off")

        for ax, node in zip(axes.flat, nodes):
            if len(root_vertices) >= 3:
                ax.fill(
                    [v[0] for v in root_vertices],
                    [v[1] for v in root_vertices],
                    color="#DDDDDD",
                )

            branch_constraints = cls.node_constraints(tree, node["id"])
            vertices = cls.region_vertices(constraints + branch_constraints, limit)
            color = STATUS_COLORS[node["status"]]

            if len(vertices) >= 3:
                ax.fill(
                    [v[0] for v in vertices],
                    [v[1] for v in vertices],
                    color=color,
                    alpha=0.45,
                )

            for constraint in branch_constraints:
                cls.draw_line(ax, constraint, limit, color="black", linestyle="--", linewidth=1)

            if node["status"] not in (3, 4):
                ax.scatter(*node["x"], color="black", marker="X", s=50, zorder=3)

            title = f"N{node['id']}"
            if branch_constraints:
                title += ": " + ", ".join(
                    cls.format_branch(tree[i], variables)
                    for i in cls.node_path(tree, node["id"])
                )
            subtitle = (
                STATUS_NAMES[node["status"]]
                if node["status"] in (3, 4)
                else f"z = {cls.format_number(node['z'])} ({STATUS_NAMES[node['status']]}"
                + (", best)" if node["id"] == best_id else ")")
            )
            ax.set_title(f"{title}\n{subtitle}", fontsize=9)
            ax.set_xlim(0, limit)
            ax.set_ylim(0, limit)
            ax.set_xlabel(variables[0], fontsize=8)
            ax.set_ylabel(variables[1], fontsize=8)
            ax.tick_params(labelsize=7)
            ax.grid(alpha=0.3)

        fig.suptitle("Feasible Area of every Subproblem")
        fig.tight_layout()
        fig.savefig(path, dpi=150)
        plt.close(fig)

    @classmethod
    def node_path(cls, tree: List[Dict], node_id: int) -> List[int]:
        path: List[int] = []
        node = tree[node_id]
        while node["parent"] != -1:
            path.append(node["id"])
            node = tree[node["parent"]]
        return list(reversed(path))
