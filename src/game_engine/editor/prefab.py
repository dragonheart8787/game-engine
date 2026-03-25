"""Prefab I/O helpers for editor/runtime interop."""

from __future__ import annotations

import json
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any


@dataclass(slots=True)
class Prefab:
    prefab_id: str
    version: int
    components: dict[str, dict[str, Any]]


def serialize_prefab(prefab: Prefab) -> str:
    return json.dumps(asdict(prefab), sort_keys=True)


def deserialize_prefab(payload: str) -> Prefab:
    raw = json.loads(payload)
    return Prefab(
        prefab_id=raw["prefab_id"],
        version=int(raw["version"]),
        components=dict(raw.get("components", {})),
    )


def save_prefab(prefab: Prefab, path: str | Path) -> None:
    Path(path).write_text(serialize_prefab(prefab), encoding="utf-8")


def load_prefab(path: str | Path) -> Prefab:
    return deserialize_prefab(Path(path).read_text(encoding="utf-8"))
