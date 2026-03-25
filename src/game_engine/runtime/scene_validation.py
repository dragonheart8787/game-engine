"""Scene validation with structured diagnostics."""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum


class ValidationSeverity(str, Enum):
    ERROR = "error"
    WARNING = "warning"


@dataclass(slots=True)
class SceneValidationIssue:
    code: str
    message: str
    path: str
    severity: ValidationSeverity = ValidationSeverity.ERROR


@dataclass(slots=True)
class SceneValidationReport:
    issues: list[SceneValidationIssue] = field(default_factory=list)

    @property
    def valid(self) -> bool:
        return all(issue.severity != ValidationSeverity.ERROR for issue in self.issues)

    def add(
        self,
        code: str,
        message: str,
        path: str,
        severity: ValidationSeverity = ValidationSeverity.ERROR,
    ) -> None:
        self.issues.append(
            SceneValidationIssue(code=code, message=message, path=path, severity=severity)
        )


REQUIRED_NODE_FIELDS = {"id", "type"}


def validate_scene(scene: dict[str, object]) -> SceneValidationReport:
    report = SceneValidationReport()
    nodes = scene.get("nodes")

    if not isinstance(nodes, list):
        report.add("scene.nodes.missing", "Scene must define a list of nodes", "nodes")
        return report

    seen_ids: set[str] = set()
    for idx, node in enumerate(nodes):
        path = f"nodes[{idx}]"
        if not isinstance(node, dict):
            report.add("scene.node.invalid_type", "Node must be an object", path)
            continue

        missing = [field for field in REQUIRED_NODE_FIELDS if field not in node]
        for field in missing:
            report.add(
                "scene.node.missing_field",
                f"Node missing required field '{field}'",
                f"{path}.{field}",
            )

        node_id = node.get("id")
        if isinstance(node_id, str):
            if node_id in seen_ids:
                report.add(
                    "scene.node.duplicate_id",
                    f"Duplicate node id '{node_id}'",
                    f"{path}.id",
                )
            seen_ids.add(node_id)
        else:
            report.add("scene.node.id.invalid", "Node id must be a string", f"{path}.id")

        components = node.get("components", [])
        if not isinstance(components, list):
            report.add(
                "scene.node.components.invalid",
                "Node components must be a list",
                f"{path}.components",
            )

    return report
