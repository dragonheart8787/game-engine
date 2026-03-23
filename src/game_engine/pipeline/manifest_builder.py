"""Deterministic manifest builder."""

from __future__ import annotations

import hashlib
import json

from .schema import BuildManifest


def manifest_hash(manifest: BuildManifest) -> str:
    normalized = {
        "commit_sha": manifest.commit_sha,
        "toolchain": manifest.toolchain,
        "platform": manifest.platform,
        "input_hashes": sorted(manifest.input_hashes),
    }
    return hashlib.sha256(json.dumps(normalized, sort_keys=True).encode("utf-8")).hexdigest()
