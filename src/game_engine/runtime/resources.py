"""Asynchronous runtime resource management."""

from __future__ import annotations

import asyncio
import contextlib
from dataclasses import dataclass
from enum import Enum
from typing import Any, Awaitable, Callable


class ResourceStatus(str, Enum):
    UNLOADED = "unloaded"
    LOADING = "loading"
    READY = "ready"
    ERROR = "error"


LoaderFn = Callable[[str], Awaitable[Any]]


@dataclass(slots=True)
class ResourceRecord:
    key: str
    loader: LoaderFn
    status: ResourceStatus = ResourceStatus.UNLOADED
    value: Any = None
    error: str | None = None
    task: asyncio.Task[Any] | None = None


class ResourceManager:
    """Async loader registry with de-duplication and lifecycle management."""

    def __init__(self) -> None:
        self._records: dict[str, ResourceRecord] = {}
        self._lock = asyncio.Lock()

    def register(self, key: str, loader: LoaderFn) -> None:
        if key in self._records:
            raise ValueError(f"Resource '{key}' already registered")
        self._records[key] = ResourceRecord(key=key, loader=loader)

    async def load(self, key: str) -> Any:
        record = self._records[key]
        async with self._lock:
            if record.status == ResourceStatus.READY:
                return record.value
            if record.task is None or record.task.done():
                record.status = ResourceStatus.LOADING
                record.error = None
                record.task = asyncio.create_task(record.loader(key))
            task = record.task

        try:
            value = await task
        except Exception as exc:  # pragma: no cover - explicit state flow
            async with self._lock:
                record.status = ResourceStatus.ERROR
                record.error = str(exc)
            raise

        async with self._lock:
            record.status = ResourceStatus.READY
            record.value = value
            record.task = None
        return value

    def get(self, key: str) -> Any:
        record = self._records[key]
        if record.status != ResourceStatus.READY:
            raise RuntimeError(f"Resource '{key}' is not ready")
        return record.value

    async def unload(self, key: str) -> None:
        record = self._records[key]
        if record.task and not record.task.done():
            record.task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await record.task
        record.task = None
        record.value = None
        record.error = None
        record.status = ResourceStatus.UNLOADED

    def status(self, key: str) -> ResourceStatus:
        return self._records[key].status

    def evict_errors(self) -> int:
        removed = 0
        for record in self._records.values():
            if record.status == ResourceStatus.ERROR:
                record.status = ResourceStatus.UNLOADED
                record.error = None
                record.task = None
                removed += 1
        return removed
