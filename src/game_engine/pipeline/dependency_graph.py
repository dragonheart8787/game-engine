"""Pipeline dependency graph and ordering support."""

from __future__ import annotations

from collections import defaultdict, deque


class DependencyGraph:
    def __init__(self) -> None:
        self._nodes: set[str] = set()
        self._edges: dict[str, set[str]] = defaultdict(set)

    def add_node(self, node: str) -> None:
        self._nodes.add(node)

    def add_dependency(self, asset: str, depends_on: str) -> None:
        self._nodes.add(asset)
        self._nodes.add(depends_on)
        self._edges[asset].add(depends_on)

    def dependencies_of(self, node: str) -> tuple[str, ...]:
        return tuple(sorted(self._edges.get(node, set())))

    def topological_order(self) -> list[str]:
        indegree = {node: 0 for node in self._nodes}
        reverse_edges: dict[str, set[str]] = defaultdict(set)
        for node, deps in self._edges.items():
            indegree[node] += len(deps)
            for dep in deps:
                reverse_edges[dep].add(node)

        queue = deque(sorted(node for node, degree in indegree.items() if degree == 0))
        order: list[str] = []
        while queue:
            node = queue.popleft()
            order.append(node)
            for consumer in sorted(reverse_edges.get(node, set())):
                indegree[consumer] -= 1
                if indegree[consumer] == 0:
                    queue.append(consumer)

        if len(order) != len(self._nodes):
            raise ValueError("Dependency graph contains a cycle")
        return order
