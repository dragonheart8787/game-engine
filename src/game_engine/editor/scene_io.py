"""Scene authoring IO helpers."""

from __future__ import annotations

import json
from pathlib import Path

from .contracts import SceneAsset


def create_scene(scene_guid: str, version: int = 1) -> SceneAsset:
    return SceneAsset(scene_guid=scene_guid, version=version, root_entities=[])


def add_entity(scene: SceneAsset, entity_id: str) -> SceneAsset:
    return SceneAsset(scene_guid=scene.scene_guid, version=scene.version, root_entities=[*scene.root_entities, entity_id])


def save_scene(scene: SceneAsset, path: str) -> None:
    Path(path).write_text(
        json.dumps(
            {
                "scene_guid": scene.scene_guid,
                "version": scene.version,
                "root_entities": scene.root_entities,
            },
            indent=2,
            sort_keys=True,
        ),
        encoding="utf-8",
    )


def load_scene(path: str) -> SceneAsset:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    return SceneAsset(
        scene_guid=data["scene_guid"],
        version=int(data["version"]),
        root_entities=list(data.get("root_entities", [])),
    )
