"""Pipeline verification helpers for reproducibility and patch compatibility."""

from __future__ import annotations

import json
from pathlib import Path


def verify_reproducible_manifest(path_a: str, path_b: str) -> bool:
    a = json.loads(Path(path_a).read_text(encoding="utf-8"))
    b = json.loads(Path(path_b).read_text(encoding="utf-8"))
    return a == b


def parse_semver(version: str) -> tuple[int, int, int]:
    parts = version.split(".")
    if len(parts) != 3:
        raise ValueError("version must follow semver MAJOR.MINOR.PATCH")
    return int(parts[0]), int(parts[1]), int(parts[2])


def verify_patch_compatibility(base_version: str, target_version: str) -> bool:
    base = parse_semver(base_version)
    target = parse_semver(target_version)
    return base[0] == target[0] and target >= base
