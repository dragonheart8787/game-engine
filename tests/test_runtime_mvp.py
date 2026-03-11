from game_engine.runtime.contracts import EngineContext
from game_engine.runtime.engine import RuntimeEngine
from game_engine.runtime.renderer import MockRenderer


def test_runtime_engine_runs_frames() -> None:
    renderer = MockRenderer()
    engine = RuntimeEngine(renderer=renderer)
    engine.initialize(EngineContext(startup_scene_id="examples/minimal-game/scene_boot.json", build_id="dev"))

    first = engine.tick(16.6)
    second = engine.tick(16.6)

    assert first["frame_index"] == 1
    assert second["frame_index"] == 2
    assert second["draw_calls"] == 4
    assert len(renderer.calls) == 2
