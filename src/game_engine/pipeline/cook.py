"""Asset cook/compress/transcode pipeline."""

from __future__ import annotations

import gzip
import json
from pathlib import Path


def cook_asset(source_path: str, output_path: str, platform: str) -> str:
    payload = {
        "platform": platform,
        "source": source_path,
        "content": Path(source_path).read_text(encoding="utf-8"),
    }
    out = Path(output_path)
    out.parent.mkdir(parents=True, exist_ok=True)
    with gzip.open(out, "wb") as f:
        f.write(json.dumps(payload, sort_keys=True).encode("utf-8"))
    return str(out)


def transcode_texture(profile: str) -> str:
    mapping = {
        "mobile": "ASTC",
        "desktop": "BC7",
        "switch": "BC3",
    }
    return mapping.get(profile, "ETC2")
