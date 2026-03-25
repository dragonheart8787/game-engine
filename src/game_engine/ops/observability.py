"""Observability metrics collection and export helpers."""

from __future__ import annotations

import json
from dataclasses import dataclass, field


@dataclass(slots=True)
class MetricStore:
    counters: dict[str, int] = field(default_factory=dict)
    gauges: dict[str, float] = field(default_factory=dict)
    events: list[dict[str, str]] = field(default_factory=list)

    def inc(self, name: str, value: int = 1) -> None:
        self.counters[name] = self.counters.get(name, 0) + value

    def set_gauge(self, name: str, value: float) -> None:
        self.gauges[name] = value

    def emit(self, name: str, **fields: str) -> None:
        event = {"name": name, **fields}
        self.events.append(event)

    def to_dict(self) -> dict[str, object]:
        return {
            "counters": dict(sorted(self.counters.items())),
            "gauges": dict(sorted(self.gauges.items())),
            "events": list(self.events),
        }


def dump_metrics(metrics: MetricStore) -> str:
    return json.dumps(metrics.to_dict(), sort_keys=True)
