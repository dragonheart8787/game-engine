"""Simple benchmark helpers with multi-round support."""

from __future__ import annotations

from dataclasses import dataclass
from statistics import mean
from time import perf_counter
from typing import Callable


BenchmarkFn = Callable[[], None]


@dataclass(slots=True)
class BenchmarkReport:
    rounds: int
    durations_ms: list[float]
    min_ms: float
    max_ms: float
    avg_ms: float


def run_benchmark(fn: BenchmarkFn, rounds: int = 1) -> BenchmarkReport:
    if rounds <= 0:
        raise ValueError("rounds must be positive")

    durations: list[float] = []
    for _ in range(rounds):
        start = perf_counter()
        fn()
        end = perf_counter()
        durations.append((end - start) * 1000.0)

    return BenchmarkReport(
        rounds=rounds,
        durations_ms=durations,
        min_ms=min(durations),
        max_ms=max(durations),
        avg_ms=mean(durations),
    )
