"""Patch/DLC release and crash ingestion flow."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path


@dataclass(slots=True)
class ReleaseArtifact:
    version: str
    base_version: str
    payload_path: str


def make_patch_manifest(base_version: str, target_version: str, files: list[str], out_path: str) -> str:
    payload = {
        "base_version": base_version,
        "target_version": target_version,
        "files": sorted(files),
        "kind": "patch",
    }
    path = Path(out_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    return str(path)


def make_dlc_manifest(version: str, dlc_id: str, files: list[str], out_path: str) -> str:
    payload = {"version": version, "dlc_id": dlc_id, "files": sorted(files), "kind": "dlc"}
    path = Path(out_path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    return str(path)


def ingest_crash(payload: dict[str, object], out_dir: str) -> str:
    build = str(payload.get("build_id", "unknown"))
    fingerprint = str(payload.get("stack_fingerprint", "none"))
    path = Path(out_dir) / f"{build}-{fingerprint}.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, sort_keys=True), encoding="utf-8")
    return str(path)
