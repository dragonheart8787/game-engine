"""Runtime configuration loading and override helpers."""

from __future__ import annotations

import json
from pathlib import Path


def load_config(path: str) -> dict[str, object]:
    return json.loads(Path(path).read_text(encoding="utf-8"))


def apply_override(config: dict[str, object], overrides: dict[str, object]) -> dict[str, object]:
    merged = dict(config)
    merged.update(overrides)
    return merged
