"""Runtime subsystem contracts and lifecycle."""

from dataclasses import dataclass
from enum import Enum


class Subsystem(str, Enum):
    LOGGING = "logging"
    ASSET = "asset"
    RENDER = "render"
    SCENE = "scene"
    PHYSICS = "physics"
    AUDIO = "audio"
    INPUT = "input"
    UI = "ui"
    SCRIPTING = "scripting"
    NETWORK = "network"


REGISTRATION_ORDER: tuple[Subsystem, ...] = (
    Subsystem.LOGGING,
    Subsystem.ASSET,
    Subsystem.RENDER,
    Subsystem.SCENE,
    Subsystem.PHYSICS,
    Subsystem.AUDIO,
    Subsystem.INPUT,
    Subsystem.UI,
    Subsystem.SCRIPTING,
    Subsystem.NETWORK,
)


@dataclass(slots=True)
class EngineContext:
    startup_scene_id: str
    build_id: str


@dataclass(slots=True)
class FrameContext:
    frame_time_ms: float
    frame_index: int


class EngineLoop:
    """Minimal executable lifecycle contract."""

    def __init__(self) -> None:
        self.initialized = False
        self.loaded_scene: str | None = None

    def initialize(self, context: EngineContext) -> None:
        if self.initialized:
            raise RuntimeError("Engine already initialized")
        self.loaded_scene = context.startup_scene_id
        self.initialized = True

    def tick(self, frame: FrameContext) -> dict[str, float | int]:
        if not self.initialized:
            raise RuntimeError("Engine not initialized")
        return {"frame_time_ms": frame.frame_time_ms, "frame_index": frame.frame_index}

    def shutdown(self) -> None:
        self.initialized = False
        self.loaded_scene = None
