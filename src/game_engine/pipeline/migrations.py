"""Pipeline state migrations and verification routines."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class PipelineState:
    schema_version: int
    records: list[dict[str, str]]


def _migrate_v1_to_v2(state: PipelineState) -> PipelineState:
    migrated = []
    for record in state.records:
        out = dict(record)
        out.setdefault("compression", "none")
        migrated.append(out)
    return PipelineState(schema_version=2, records=migrated)


MIGRATIONS: dict[int, callable] = {1: _migrate_v1_to_v2}


def migrate_pipeline_state(state: PipelineState, target_version: int) -> PipelineState:
    current = state
    while current.schema_version < target_version:
        step = MIGRATIONS.get(current.schema_version)
        if step is None:
            raise ValueError(f"No migration from schema v{current.schema_version}")
        current = step(current)
    return current


def verify_pipeline_state(state: PipelineState) -> list[str]:
    issues: list[str] = []
    if state.schema_version <= 0:
        issues.append("schema_version must be positive")
    for idx, record in enumerate(state.records):
        if "guid" not in record:
            issues.append(f"record[{idx}] missing guid")
        if "hash" not in record:
            issues.append(f"record[{idx}] missing hash")
    return issues
