"""Simple deterministic physics step."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class RigidBody:
    position_y: float
    velocity_y: float
    mass: float = 1.0


def step(body: RigidBody, dt_seconds: float, gravity: float = -9.81) -> RigidBody:
    new_velocity = body.velocity_y + gravity * dt_seconds
    new_position = body.position_y + new_velocity * dt_seconds
    return RigidBody(position_y=new_position, velocity_y=new_velocity, mass=body.mass)
