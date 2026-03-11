"""Runtime audio mixer layer."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class AudioTrack:
    name: str
    volume: float
    pan: float


def mix(tracks: list[AudioTrack]) -> dict[str, float]:
    if not tracks:
        return {"left": 0.0, "right": 0.0}
    left = sum(max(0.0, 1.0 - t.pan) * t.volume for t in tracks)
    right = sum(max(0.0, 1.0 + t.pan) * t.volume for t in tracks)
    return {"left": round(left, 4), "right": round(right, 4)}
