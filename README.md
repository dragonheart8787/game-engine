# game-engine-platform

Python executable scaffold for game-engine platform contracts.

## Compatibility policy

Plugin compatibility is determined by matching **major** engine version only.
Version strings may include an optional leading `v`/`V` (for example `v1.4.0`).
Malformed version strings (empty, non-numeric major, or non-semver-like labels)
are treated as **incompatible**.

## Actionable Roadmap

1. Stabilize runtime lifecycle contracts and enforce guardrail behavior through tests.
2. Expand plugin contract validation to include richer semver parsing and diagnostics.
3. Add service, pipeline, and governance contract fixtures for integration-level verification.

## Run checks

```bash
scripts/check-structure.sh
scripts/lint-docs.sh
pytest
```
