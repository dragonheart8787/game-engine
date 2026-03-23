"""Simple runtime resource manager with ref counting."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class ResourceEntry:
    key: str
    refs: int = 0


class ResourceManager:
    def __init__(self) -> None:
        self._resources: dict[str, ResourceEntry] = {}

    def acquire(self, key: str) -> None:
        entry = self._resources.get(key)
        if entry is None:
            entry = ResourceEntry(key=key, refs=0)
            self._resources[key] = entry
        entry.refs += 1

    def release(self, key: str) -> None:
        entry = self._resources.get(key)
        if entry is None:
            return
        entry.refs = max(0, entry.refs - 1)
        if entry.refs == 0:
            self._resources.pop(key, None)

    def stats(self) -> dict[str, int]:
        total_refs = sum(entry.refs for entry in self._resources.values())
        return {
            "resources": len(self._resources),
            "resource_refs": total_refs,
        }
