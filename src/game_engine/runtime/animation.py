"""Animation blending helpers."""

from __future__ import annotations


def blend(a: float, b: float, alpha: float) -> float:
    alpha_clamped = min(1.0, max(0.0, alpha))
    return (1.0 - alpha_clamped) * a + alpha_clamped * b
