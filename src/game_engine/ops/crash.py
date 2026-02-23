"""Crash reporting payload models."""

from dataclasses import dataclass


@dataclass(slots=True)
class CrashPayload:
    build_id: str
    platform: str
    stack_fingerprint: str
    symbol_version: str
    session_metadata: dict[str, str]
