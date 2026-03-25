import asyncio

from game_engine.runtime.ecs import EntityManager, SystemRegistry
from game_engine.runtime.resources import ResourceManager, ResourceStatus
from game_engine.runtime.scene_validation import validate_scene
from game_engine.runtime.systems import (
    AnimationState,
    AudioState,
    PhysicsBody,
    UiState,
    animation_system,
    audio_system,
    physics_system,
    ui_system,
)


def test_entity_manager_reuses_slot_with_new_generation() -> None:
    manager = EntityManager()
    first = manager.create()
    manager.destroy(first)
    second = manager.create()

    assert first.index == second.index
    assert first.generation != second.generation
    assert not manager.alive(first)
    assert manager.alive(second)


def test_system_registry_runs_in_order() -> None:
    registry = SystemRegistry()
    calls: list[str] = []

    registry.register("physics", lambda *_: calls.append("physics"))
    registry.register("animation", lambda *_: calls.append("animation"))
    registry.run(0.016, {})

    assert calls == ["physics", "animation"]


def test_runtime_systems_update_world_state() -> None:
    body = PhysicsBody(position=0.0, velocity=1.0, acceleration=1.0)
    animation = AnimationState(clip_length=1.0, time=0.9, speed=1.0)
    audio = AudioState(queued_events=["hit", "jump"], played_events=[])
    ui = UiState(dirty=True)
    world: dict[str, object] = {
        "physics_bodies": [body],
        "animation_states": [animation],
        "audio": audio,
        "ui": ui,
    }

    physics_system(0.5, world)
    animation_system(0.5, world)
    audio_system(0.5, world)
    ui_system(0.5, world)

    assert body.position > 0.5
    assert 0 <= animation.time < 1.0
    assert audio.played_events == ["hit", "jump"]
    assert ui.layout_passes == 1
    assert not ui.dirty


def test_scene_validation_reports_structured_issues() -> None:
    report = validate_scene({"nodes": [{"id": "player", "type": "actor"}, {"id": "player"}]})

    assert not report.valid
    assert any(issue.code == "scene.node.duplicate_id" for issue in report.issues)
    assert any(issue.path == "nodes[1].type" for issue in report.issues)


def test_resource_manager_async_lifecycle() -> None:
    manager = ResourceManager()

    async def fake_loader(key: str) -> str:
        await asyncio.sleep(0)
        return f"payload:{key}"

    async def scenario() -> None:
        manager.register("texture:ui", fake_loader)
        value = await manager.load("texture:ui")
        assert value == "payload:texture:ui"
        assert manager.get("texture:ui") == value
        assert manager.status("texture:ui") == ResourceStatus.READY
        await manager.unload("texture:ui")
        assert manager.status("texture:ui") == ResourceStatus.UNLOADED

    asyncio.run(scenario())


def test_resource_manager_error_state_can_be_evicted() -> None:
    manager = ResourceManager()

    async def broken_loader(_: str) -> str:
        await asyncio.sleep(0)
        raise RuntimeError("boom")

    async def scenario() -> None:
        manager.register("broken", broken_loader)
        try:
            await manager.load("broken")
        except RuntimeError:
            pass
        assert manager.status("broken") == ResourceStatus.ERROR
        assert manager.evict_errors() == 1
        assert manager.status("broken") == ResourceStatus.UNLOADED

    asyncio.run(scenario())
