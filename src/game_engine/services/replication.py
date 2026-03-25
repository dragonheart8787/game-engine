"""Replication service built on top of StateStore."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Callable

from .state_store import StateRecord, StateStore


@dataclass(frozen=True, slots=True)
class ReplicationEvent:
    key: str
    value: Any
    version: int


Subscriber = Callable[[ReplicationEvent], None]


class ReplicationService:
    def __init__(self, store: StateStore) -> None:
        self._store = store
        self._subscribers: list[Subscriber] = []

    def subscribe(self, callback: Subscriber) -> None:
        self._subscribers.append(callback)

    def replicate(self, key: str, value: Any) -> ReplicationEvent:
        record: StateRecord = self._store.put(key, value)
        event = ReplicationEvent(key=record.key, value=record.value, version=record.version)
        for callback in self._subscribers:
            callback(event)
        return event

    def replay(self) -> list[ReplicationEvent]:
        return [
            ReplicationEvent(key=record.key, value=record.value, version=record.version)
            for record in self._store.snapshot().values()
        ]
