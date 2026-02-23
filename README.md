# game-engine

A production-oriented **Game Development Platform** scaffold that defines:

- Runtime Core (engine)
- Editor & Authoring (tools)
- Asset/Data Pipeline
- Build/Release/Ops
- Cloud/Online Services
- Ecosystem (plugins/packages/marketplace)
- Governance & Compliance

> This repository is a strong starter skeleton intended to align teams before deep implementation begins.

## Repository Layout

- `engine/runtime/` — runtime subsystems and interfaces
- `editor/` — editor toolchain module boundaries
- `pipeline/` — asset database/import/build/package/patch specs
- `services/` — service contracts for auth, matchmaking, inventory, liveops
- `ecosystem/` — plugin SDK and package/marketplace contracts
- `docs/` — architecture, roadmap, operational and compliance docs
- `scripts/` — local developer and CI helper scripts
- `.github/workflows/` — baseline CI checks
- `examples/` — sample projects and regression targets
- `tests/` — architecture/documentation contract checks

## Quick Start

```bash
scripts/check-structure.sh
scripts/lint-docs.sh
```

## MVP Delivery Milestones

See `docs/roadmap/mvp-milestones.md`.
