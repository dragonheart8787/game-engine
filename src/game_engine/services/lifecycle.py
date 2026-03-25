"""Service lifecycle helpers including graceful shutdown."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Callable


ShutdownHook = Callable[[], None]


@dataclass(slots=True)
class GracefulShutdown:
    _hooks: list[ShutdownHook] = field(default_factory=list)
    _closed: bool = False

    def register(self, hook: ShutdownHook) -> None:
        if self._closed:
            raise RuntimeError("Shutdown already in progress")
        self._hooks.append(hook)

    def shutdown(self) -> None:
        if self._closed:
            return
        self._closed = True
        errors: list[str] = []
        for hook in reversed(self._hooks):
            try:
                hook()
            except Exception as exc:  # pragma: no cover - explicit failure path
                errors.append(str(exc))
        if errors:
            raise RuntimeError("; ".join(errors))

    @property
    def closed(self) -> bool:
        return self._closed
