"""Entity-component-system foundational runtime primitives."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable


@dataclass(frozen=True, slots=True)
class EntityId:
    """Opaque entity handle with generation tracking."""

    index: int
    generation: int


class EntityManager:
    """Allocates entity ids and tracks liveness."""

    def __init__(self) -> None:
        self._generations: list[int] = []
        self._free: list[int] = []

    def create(self) -> EntityId:
        if self._free:
            index = self._free.pop()
            generation = self._generations[index]
        else:
            index = len(self._generations)
            generation = 0
            self._generations.append(generation)
        return EntityId(index=index, generation=generation)

    def destroy(self, entity: EntityId) -> None:
        if not self.alive(entity):
            raise ValueError(f"Unknown or stale entity {entity}")
        self._generations[entity.index] += 1
        self._free.append(entity.index)

    def alive(self, entity: EntityId) -> bool:
        if entity.index < 0 or entity.index >= len(self._generations):
            return False
        return self._generations[entity.index] == entity.generation


SystemFn = Callable[[float, dict[str, Any]], None]


class SystemRegistry:
    """Ordered registry for update systems."""

    def __init__(self) -> None:
        self._systems: dict[str, SystemFn] = {}
        self._order: list[str] = []

    def register(self, name: str, system: SystemFn) -> None:
        if name in self._systems:
            raise ValueError(f"System '{name}' already registered")
        self._systems[name] = system
        self._order.append(name)

    def unregister(self, name: str) -> None:
        if name not in self._systems:
            raise KeyError(name)
        del self._systems[name]
        self._order.remove(name)

    def get(self, name: str) -> SystemFn:
        return self._systems[name]

    def names(self) -> tuple[str, ...]:
        return tuple(self._order)

    def run(self, dt_seconds: float, world: dict[str, Any]) -> None:
        for name in self._order:
            self._systems[name](dt_seconds, world)
