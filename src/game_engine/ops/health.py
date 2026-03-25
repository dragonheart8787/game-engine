"""Health operations and checks for service layer."""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Callable


class HealthStatus(str, Enum):
    OK = "ok"
    DEGRADED = "degraded"
    FAIL = "fail"


HealthCheck = Callable[[], HealthStatus]


@dataclass(frozen=True, slots=True)
class HealthIssue:
    name: str
    status: HealthStatus


@dataclass(slots=True)
class HealthReport:
    overall: HealthStatus
    issues: list[HealthIssue]


class HealthRegistry:
    def __init__(self) -> None:
        self._checks: dict[str, HealthCheck] = {}

    def register(self, name: str, check: HealthCheck) -> None:
        if name in self._checks:
            raise ValueError(f"Health check '{name}' already exists")
        self._checks[name] = check

    def run(self) -> HealthReport:
        issues: list[HealthIssue] = []
        overall = HealthStatus.OK
        for name, check in self._checks.items():
            status = check()
            if status != HealthStatus.OK:
                issues.append(HealthIssue(name=name, status=status))
            if status == HealthStatus.FAIL:
                overall = HealthStatus.FAIL
            elif status == HealthStatus.DEGRADED and overall == HealthStatus.OK:
                overall = HealthStatus.DEGRADED
        return HealthReport(overall=overall, issues=issues)
