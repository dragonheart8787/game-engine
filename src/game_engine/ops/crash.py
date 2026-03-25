"""Crash reporting payload models and fingerprint aggregation."""

from __future__ import annotations

import hashlib
from dataclasses import dataclass


@dataclass(slots=True)
class CrashPayload:
    build_id: str
    platform: str
    stack_fingerprint: str
    symbol_version: str
    session_metadata: dict[str, str]


def calculate_stack_fingerprint(stack_lines: list[str], symbol_version: str) -> str:
    """Create a normalized stable fingerprint from symbolized stack lines."""
    normalized = "|".join(line.strip().lower() for line in stack_lines if line.strip())
    digest = hashlib.sha256(f"{symbol_version}:{normalized}".encode("utf-8")).hexdigest()
    return digest[:16]


def payload_fingerprint(payload: CrashPayload) -> str:
    """Fingerprint crash payload by key envelope fields."""
    metadata_part = "|".join(f"{k}={v}" for k, v in sorted(payload.session_metadata.items()))
    raw = (
        f"{payload.build_id}|{payload.platform}|{payload.stack_fingerprint}|"
        f"{payload.symbol_version}|{metadata_part}"
    )
    return hashlib.sha256(raw.encode("utf-8")).hexdigest()[:20]


def aggregate_crashes(payloads: list[CrashPayload]) -> dict[str, int]:
    """Count crash occurrences by payload fingerprint."""
    out: dict[str, int] = {}
    for payload in payloads:
        key = payload_fingerprint(payload)
        out[key] = out.get(key, 0) + 1
    return out
