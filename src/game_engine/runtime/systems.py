"""Core runtime systems for integrated simulation/update."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class PhysicsBody:
    position: float = 0.0
    velocity: float = 0.0
    acceleration: float = 0.0


@dataclass(slots=True)
class AnimationState:
    clip_length: float
    time: float = 0.0
    speed: float = 1.0


@dataclass(slots=True)
class AudioState:
    queued_events: list[str]
    played_events: list[str]


@dataclass(slots=True)
class UiState:
    dirty: bool = False
    layout_passes: int = 0


def physics_system(dt_seconds: float, world: dict[str, object]) -> None:
    for body in world.get("physics_bodies", []):
        assert isinstance(body, PhysicsBody)
        body.velocity += body.acceleration * dt_seconds
        body.position += body.velocity * dt_seconds


def animation_system(dt_seconds: float, world: dict[str, object]) -> None:
    for state in world.get("animation_states", []):
        assert isinstance(state, AnimationState)
        if state.clip_length <= 0:
            continue
        state.time = (state.time + dt_seconds * state.speed) % state.clip_length


def audio_system(_: float, world: dict[str, object]) -> None:
    audio = world.get("audio")
    if audio is None:
        return
    assert isinstance(audio, AudioState)
    while audio.queued_events:
        audio.played_events.append(audio.queued_events.pop(0))


def ui_system(_: float, world: dict[str, object]) -> None:
    ui = world.get("ui")
    if ui is None:
        return
    assert isinstance(ui, UiState)
    if ui.dirty:
        ui.layout_passes += 1
        ui.dirty = False
