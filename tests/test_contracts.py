from game_engine.architecture.platform import PLATFORM_PILLARS
from game_engine.ecosystem.contracts import is_compatible
from game_engine.editor.contracts import ImportRequest, needs_reimport
from game_engine.runtime.contracts import EngineContext, EngineLoop, FrameContext


def test_engine_loop_lifecycle() -> None:
    loop = EngineLoop()
    loop.initialize(EngineContext(startup_scene_id="boot", build_id="dev"))
    out = loop.tick(FrameContext(frame_time_ms=16.6, frame_index=1))
    assert out["frame_index"] == 1
    loop.shutdown()


def test_engine_loop_requires_initialize() -> None:
    loop = EngineLoop()
    try:
        loop.tick(FrameContext(frame_time_ms=16.6, frame_index=1))
    except RuntimeError as exc:
        assert str(exc) == "Engine not initialized"
    else:  # pragma: no cover - explicit failure branch
        raise AssertionError("Expected RuntimeError before initialize")


def test_engine_loop_rejects_double_initialize() -> None:
    loop = EngineLoop()
    loop.initialize(EngineContext(startup_scene_id="boot", build_id="dev"))
    try:
        loop.initialize(EngineContext(startup_scene_id="boot", build_id="dev"))
    except RuntimeError as exc:
        assert str(exc) == "Engine already initialized"
    else:  # pragma: no cover - explicit failure branch
        raise AssertionError("Expected RuntimeError on second initialize")


def test_reimport_decision() -> None:
    req = ImportRequest(source_path="a.glb", source_hash="2", settings_hash="1")
    assert needs_reimport("1", "1", req)


def test_plugin_compatibility() -> None:
    assert is_compatible("1.3.0", "1.0.0")
    assert not is_compatible("2.0.0", "1.9.0")


def test_plugin_compatibility_tolerates_prefix_and_invalid_input() -> None:
    assert is_compatible("v1.4.0", "1.0.0")
    assert is_compatible(" V1.4.0 ", "1.0.0")
    assert not is_compatible("", "1.0.0")
    assert not is_compatible("main", "1.0.0")


def test_platform_pillars_exist() -> None:
    assert "runtime_core" in PLATFORM_PILLARS
