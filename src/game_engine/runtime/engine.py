"""Executable runtime engine orchestrator."""

from __future__ import annotations

from dataclasses import dataclass

from .contracts import EngineContext, FrameContext
from .gameplay import (
    GameplayState,
    apply_input,
    distance_to_goal,
    initialize_gameplay,
    update_collectibles,
    update_win_condition,
)
from .renderer import MockRenderer, RenderStats
from .resources import ResourceManager
from .scheduler import FrameScheduler
from .scene_store import LoadedScene, load_scene


@dataclass(slots=True)
class RuntimeState:
    loaded_scene: LoadedScene
    gameplay: GameplayState
    frame_index: int = 0


class RuntimeEngine:
    def __init__(self, renderer: MockRenderer | None = None) -> None:
        self.renderer = renderer or MockRenderer()
        self.scheduler = FrameScheduler()
        self.resources = ResourceManager()
        self.state: RuntimeState | None = None

    def initialize(self, context: EngineContext) -> None:
        scene = load_scene(context.startup_scene_id)
        gameplay = initialize_gameplay(scene)
        self.state = RuntimeState(loaded_scene=scene, gameplay=gameplay)
        for entity in scene.entities:
            self.resources.acquire(f"entity:{entity.entity_id}")

    def tick(self, frame_time_ms: float, input_action: str = "idle") -> dict[str, float | int | bool]:
        if self.state is None:
            raise RuntimeError("RuntimeEngine not initialized")

        self.state.frame_index += 1
        frame = FrameContext(frame_time_ms=frame_time_ms, frame_index=self.state.frame_index)
        dt = frame_time_ms / 1000.0

        latest_stats: RenderStats | None = None

        def _input_phase() -> None:
            apply_input(self.state.gameplay, input_action, dt_seconds=dt)

        def _gameplay_phase() -> None:
            update_collectibles(self.state.gameplay)
            update_win_condition(self.state.gameplay)

        def _render_phase() -> None:
            nonlocal latest_stats
            latest_stats = self.renderer.render(self.state.loaded_scene, frame.frame_index)

        phase_stats = self.scheduler.run_frame(
            {
                "input": _input_phase,
                "gameplay": _gameplay_phase,
                "render": _render_phase,
            }
        )

        if latest_stats is None:
            raise RuntimeError("Render phase did not execute")

        return self._to_result(frame, latest_stats, phase_stats)

    def _to_result(self, frame: FrameContext, stats: RenderStats, phase_stats: list) -> dict[str, float | int | bool]:
        gameplay = self.state.gameplay if self.state else None
        if gameplay is None:
            raise RuntimeError("Runtime state missing gameplay")

        result: dict[str, float | int | bool] = {
            "frame_index": frame.frame_index,
            "frame_time_ms": frame.frame_time_ms,
            "draw_calls": stats.draw_calls,
            "rendered_entities": stats.rendered_entities,
            "player_x": round(gameplay.player.transform.x, 4),
            "player_z": round(gameplay.player.transform.z, 4),
            "distance_to_goal": distance_to_goal(gameplay),
            "won": gameplay.won,
            "collected_coin": gameplay.collected_coin,
            "score": gameplay.score,
        }

        for stat in phase_stats:
            result[f"phase_{stat.phase}_ms"] = stat.ms
        result.update(self.resources.stats())
        return result
