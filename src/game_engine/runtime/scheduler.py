"""Runtime phase scheduler for deterministic frame execution."""

from __future__ import annotations

from dataclasses import dataclass
from time import perf_counter
from typing import Callable

PhaseFn = Callable[[], None]


@dataclass(slots=True)
class PhaseStat:
    phase: str
    ms: float


class FrameScheduler:
    """Executes phases in a stable order and records timings."""

    ORDER = ("input", "gameplay", "render")

    def run_frame(self, phases: dict[str, PhaseFn]) -> list[PhaseStat]:
        stats: list[PhaseStat] = []
        for phase_name in self.ORDER:
            fn = phases.get(phase_name)
            if fn is None:
                continue
            start = perf_counter()
            fn()
            duration_ms = (perf_counter() - start) * 1000.0
            stats.append(PhaseStat(phase=phase_name, ms=round(duration_ms, 4)))
        return stats
