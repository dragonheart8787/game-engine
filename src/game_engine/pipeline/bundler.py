"""Bundle emission helpers."""

from __future__ import annotations

import json
from dataclasses import asdict
from pathlib import Path

from .schema import BundleMetadata


def write_bundle(output_dir: str, metadata: BundleMetadata, artifacts: list[str]) -> str:
    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)
    bundle_path = out / f"{metadata.bundle_id}-{metadata.semver}.bundle.json"
    payload = {
        "metadata": asdict(metadata),
        "artifacts": sorted(artifacts),
    }
    bundle_path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")
    return str(bundle_path)
