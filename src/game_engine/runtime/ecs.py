"""Minimal ECS implementation."""

from __future__ import annotations

from dataclasses import dataclass, field


@dataclass(slots=True)
class Entity:
    entity_id: str
    components: dict[str, dict[str, float | int | str]] = field(default_factory=dict)


class World:
    def __init__(self) -> None:
        self.entities: dict[str, Entity] = {}

    def create_entity(self, entity_id: str) -> Entity:
        ent = Entity(entity_id=entity_id)
        self.entities[entity_id] = ent
        return ent

    def add_component(self, entity_id: str, component_name: str, payload: dict[str, float | int | str]) -> None:
        self.entities[entity_id].components[component_name] = payload

    def query(self, component_name: str) -> list[Entity]:
        return [e for e in self.entities.values() if component_name in e.components]
