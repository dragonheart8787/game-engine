"""Single place to advance frame simulation via a system registry and shared world."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from .ecs import SystemRegistry


@dataclass
class TickCoordinator:
    """Runs registered systems in order against one mutable world dict each tick."""

    world: dict[str, Any] = field(default_factory=dict)
    registry: SystemRegistry = field(default_factory=SystemRegistry)

    def tick(self, dt_seconds: float) -> None:
        self.registry.run(dt_seconds, self.world)
