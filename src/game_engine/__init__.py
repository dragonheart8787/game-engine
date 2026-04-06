"""Game engine platform scaffold package."""

from .architecture.platform import PLATFORM_PILLARS
from .runtime.ecs import EntityId, EntityManager, SystemRegistry
from .runtime.systems import (
    AnimationState,
    AudioState,
    PhysicsBody,
    UiState,
    animation_system,
    audio_system,
    physics_system,
    register_standard_systems,
    ui_system,
)
from .runtime.tick_coordinator import TickCoordinator
from .services.state_store import StateRecord, StateStore

__all__ = [
    "PLATFORM_PILLARS",
    "EntityId",
    "EntityManager",
    "SystemRegistry",
    "PhysicsBody",
    "AnimationState",
    "AudioState",
    "UiState",
    "physics_system",
    "animation_system",
    "audio_system",
    "ui_system",
    "register_standard_systems",
    "TickCoordinator",
    "StateRecord",
    "StateStore",
]
