"""Mock renderer implementation for MVP runtime."""

from __future__ import annotations

from dataclasses import dataclass

from .scene_store import LoadedScene


@dataclass(slots=True)
class RenderStats:
    frame_index: int
    draw_calls: int
    rendered_entities: int


class MockRenderer:
    def __init__(self) -> None:
        self.calls: list[RenderStats] = []

    def render(self, scene: LoadedScene, frame_index: int) -> RenderStats:
        stats = RenderStats(
            frame_index=frame_index,
            draw_calls=len(scene.entities),
            rendered_entities=len(scene.entities),
        )
        self.calls.append(stats)
        return stats
