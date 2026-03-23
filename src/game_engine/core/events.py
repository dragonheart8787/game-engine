"""Simple event bus for engine subsystems."""

from __future__ import annotations

from collections import defaultdict
from typing import Callable

EventHandler = Callable[[dict[str, object]], None]


class EventBus:
    def __init__(self) -> None:
        self._handlers: dict[str, list[EventHandler]] = defaultdict(list)

    def subscribe(self, event: str, handler: EventHandler) -> None:
        self._handlers[event].append(handler)

    def emit(self, event: str, payload: dict[str, object]) -> None:
        for handler in self._handlers.get(event, []):
            handler(payload)
