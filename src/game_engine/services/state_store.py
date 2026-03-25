"""In-memory state store with versioned records."""

from __future__ import annotations

from dataclasses import dataclass
from threading import RLock
from typing import Any


@dataclass(frozen=True, slots=True)
class StateRecord:
    key: str
    value: Any
    version: int


class StateStore:
    def __init__(self) -> None:
        self._items: dict[str, StateRecord] = {}
        self._lock = RLock()

    def get(self, key: str) -> StateRecord | None:
        with self._lock:
            return self._items.get(key)

    def put(self, key: str, value: Any) -> StateRecord:
        with self._lock:
            current = self._items.get(key)
            version = 1 if current is None else current.version + 1
            record = StateRecord(key=key, value=value, version=version)
            self._items[key] = record
            return record

    def compare_and_set(self, key: str, expected_version: int, value: Any) -> StateRecord:
        with self._lock:
            current = self._items.get(key)
            if current is None:
                if expected_version != 0:
                    raise ValueError("Version mismatch")
                return self.put(key, value)

            if current.version != expected_version:
                raise ValueError("Version mismatch")

            record = StateRecord(key=key, value=value, version=current.version + 1)
            self._items[key] = record
            return record

    def snapshot(self) -> dict[str, StateRecord]:
        with self._lock:
            return dict(self._items)
