from game_engine.runtime.contracts import EngineContext
from game_engine.runtime.engine import RuntimeEngine
from game_engine.runtime.resources import ResourceManager
from game_engine.runtime.scheduler import FrameScheduler


def test_scheduler_phase_order() -> None:
    scheduler = FrameScheduler()
    order: list[str] = []

    stats = scheduler.run_frame(
        {
            "input": lambda: order.append("input"),
            "gameplay": lambda: order.append("gameplay"),
            "render": lambda: order.append("render"),
        }
    )

    assert order == ["input", "gameplay", "render"]
    assert [s.phase for s in stats] == ["input", "gameplay", "render"]


def test_resource_manager_ref_count() -> None:
    resources = ResourceManager()
    resources.acquire("mesh:player")
    resources.acquire("mesh:player")
    assert resources.stats()["resources"] == 1
    assert resources.stats()["resource_refs"] == 2

    resources.release("mesh:player")
    assert resources.stats()["resource_refs"] == 1
    resources.release("mesh:player")
    assert resources.stats()["resources"] == 0


def test_runtime_tick_has_phase_and_resource_stats() -> None:
    engine = RuntimeEngine()
    engine.initialize(EngineContext(startup_scene_id="examples/minimal-game/scene_boot.json", build_id="dev"))
    out = engine.tick(16.6)
    assert "phase_input_ms" in out
    assert "phase_gameplay_ms" in out
    assert "phase_render_ms" in out
    assert out["resources"] >= 1
