"""Scene loading utilities for runtime MVP."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path


@dataclass(slots=True)
class Transform:
    x: float
    y: float
    z: float


@dataclass(slots=True)
class SceneEntity:
    entity_id: str
    transform: Transform


@dataclass(slots=True)
class LoadedScene:
    scene_id: str
    entities: list[SceneEntity]


def load_scene(path: str | Path) -> LoadedScene:
    payload = json.loads(Path(path).read_text(encoding="utf-8"))
    entities = [
        SceneEntity(
            entity_id=item["id"],
            transform=Transform(
                x=float(item["transform"]["x"]),
                y=float(item["transform"]["y"]),
                z=float(item["transform"]["z"]),
            ),
        )
        for item in payload.get("entities", [])
    ]
    return LoadedScene(scene_id=payload["scene_id"], entities=entities)
