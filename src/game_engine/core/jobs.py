"""Minimal thread-pool-backed job execution."""

from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor
from typing import Callable, TypeVar

T = TypeVar("T")


class JobSystem:
    def __init__(self, workers: int = 4) -> None:
        self._pool = ThreadPoolExecutor(max_workers=workers)

    def run(self, fn: Callable[[], T]) -> T:
        future = self._pool.submit(fn)
        return future.result()

    def shutdown(self) -> None:
        self._pool.shutdown(wait=True)
