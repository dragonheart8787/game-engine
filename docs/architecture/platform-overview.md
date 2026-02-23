# Platform Overview

## Product Goal
Build a game platform that can ship real products rather than demos.

## Pillars
1. Runtime Core: stable frame loop, extensible subsystem architecture.
2. Editor & Authoring: content teams can iterate without engine engineers.
3. Pipeline: deterministic asset processing and incremental builds.
4. Release/Ops: multi-platform packaging, telemetry, crash visibility.
5. Services: account, multiplayer, economy, liveops support.
6. Ecosystem: plugins, packages, marketplace, docs/samples.
7. Governance: licensing, security, privacy, audit.

## Non-Goals (initial MVP)
- Full AAA feature parity at launch.
- Proprietary custom replacements for every third-party middleware.

## System Boundaries
- Runtime consumes cooked assets + runtime config.
- Editor produces source assets + metadata.
- Pipeline transforms source assets into platform-specific outputs.
- Services expose APIs consumed by runtime/editor backend clients.
