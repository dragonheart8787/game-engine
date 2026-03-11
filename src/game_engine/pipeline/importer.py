"""Simple deterministic asset importer for MVP."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path


def import_asset(source_path: str, settings: dict[str, str]) -> dict[str, object]:
    source_bytes = Path(source_path).read_bytes()
    source_hash = hashlib.sha256(source_bytes).hexdigest()
    settings_hash = hashlib.sha256(json.dumps(settings, sort_keys=True).encode("utf-8")).hexdigest()
    asset_guid = hashlib.sha1(f"{source_path}:{source_hash}".encode("utf-8")).hexdigest()[:16]
    return {
        "asset_guid": asset_guid,
        "source_path": source_path,
        "source_hash": source_hash,
        "settings_hash": settings_hash,
        "artifact_refs": [f"intermediate/{asset_guid}.bin"],
    }
