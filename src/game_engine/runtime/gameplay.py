"""Core gameplay loop helpers for playability-focused runs."""

from __future__ import annotations

from dataclasses import dataclass
from math import sqrt

from .scene_store import LoadedScene, SceneEntity


@dataclass(slots=True)
class GameplayState:
    player: SceneEntity
    goal: SceneEntity
    coin: SceneEntity
    move_speed: float = 3.0
    goal_radius: float = 0.75
    coin_radius: float = 0.6
    collected_coin: bool = False
    score: int = 0
    won: bool = False


def _find_entity(scene: LoadedScene, entity_id: str) -> SceneEntity:
    for entity in scene.entities:
        if entity.entity_id == entity_id:
            return entity
    raise ValueError(f"Missing required entity '{entity_id}' in scene '{scene.scene_id}'")


def _distance(a: SceneEntity, b: SceneEntity) -> float:
    dx = a.transform.x - b.transform.x
    dy = a.transform.y - b.transform.y
    dz = a.transform.z - b.transform.z
    return sqrt(dx * dx + dy * dy + dz * dz)


def initialize_gameplay(scene: LoadedScene) -> GameplayState:
    player = _find_entity(scene, "player")
    goal = _find_entity(scene, "goal")
    coin = _find_entity(scene, "coin")
    return GameplayState(player=player, goal=goal, coin=coin)


def apply_input(state: GameplayState, action: str, dt_seconds: float) -> None:
    distance = state.move_speed * dt_seconds
    if action == "left":
        state.player.transform.x -= distance
    elif action == "right":
        state.player.transform.x += distance
    elif action == "forward":
        state.player.transform.z += distance
    elif action == "backward":
        state.player.transform.z -= distance


def update_collectibles(state: GameplayState) -> None:
    if state.collected_coin:
        return
    if _distance(state.player, state.coin) <= state.coin_radius:
        state.collected_coin = True
        state.score += 1


def update_win_condition(state: GameplayState) -> None:
    if _distance(state.player, state.goal) <= state.goal_radius and state.collected_coin:
        state.won = True


def distance_to_goal(state: GameplayState) -> float:
    return round(_distance(state.player, state.goal), 4)
