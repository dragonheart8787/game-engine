"""Immediate-mode UI tree and layout."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class Widget:
    widget_id: str
    width: int
    height: int


def vertical_layout(widgets: list[Widget], spacing: int = 8) -> dict[str, tuple[int, int]]:
    y = 0
    result: dict[str, tuple[int, int]] = {}
    for w in widgets:
        result[w.widget_id] = (0, y)
        y += w.height + spacing
    return result
