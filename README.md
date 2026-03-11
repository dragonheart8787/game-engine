# game-engine-platform

Executable scaffold for a game-engine platform with runnable runtime, backend adapters, asset cook flow, multiplayer lobby service, and release-ops utilities.

## Current Capability Snapshot

- Runtime MVP loop with scene loading + mock rendering stats
- Backend command encoders for Vulkan / DX12 / Metal
- Core runtime systems: ECS, physics step, audio mix, UI layout, animation blend
- Asset pipeline primitives: import, deterministic manifest, bundle, cook+transcode
- Service layer primitives: auth/plugin validators, async lobby matchmaking server
- Ops primitives: patch manifest, DLC manifest, crash ingestion

## Actionable Roadmap

### Phase 1 — Stabilize MVP Runtime (Now)
- Add deterministic fixed-step scheduler and frame budget instrumentation.
- Introduce scene validation and runtime error categories.
- Expand integration tests for scene + renderer + ECS interactions.

### Phase 2 — Real Asset Production Path
- Replace text-based cook with binary pack chunks and chunk table index.
- Add platform cook profiles (`windows`, `linux`, `android`, `ios`) with compression settings.
- Add build manifest signatures and artifact provenance metadata.

### Phase 3 — Service Hardening
- Add persistent lobby state store and server health probes.
- Add player session/auth handshake for multiplayer operations.
- Add deployment manifests and environment configuration templates.

### Phase 4 — LiveOps & Release Operations
- Add patch compatibility checks and rollback verification.
- Add crash ingestion deduplication and symbol mapping pipeline.
- Add telemetry export hooks for operational dashboards.

## Quick start

```bash
PYTHONPATH=src scripts/check-structure.sh
PYTHONPATH=src scripts/lint-docs.sh
PYTHONPATH=src pytest
```

## Demo run

```bash
PYTHONPATH=src python -m game_engine.cli run-demo --scene examples/minimal-game/scene_boot.json --frames 3 --frame-time 16.6
```

## Rendering backend adapters

```bash
PYTHONPATH=src python -m game_engine.cli render-backend --backend vulkan --draw-calls 4
PYTHONPATH=src python -m game_engine.cli render-backend --backend dx12 --draw-calls 4
PYTHONPATH=src python -m game_engine.cli render-backend --backend metal --draw-calls 4
```

## Content cook/transcode + release ops

```bash
PYTHONPATH=src python -m game_engine.cli cook-asset --source examples/minimal-game/scene_boot.json --out /tmp/scene.gz --platform mobile
PYTHONPATH=src python -m game_engine.cli make-patch --base 1.0.0 --target 1.0.1 --out /tmp/patch.json base.pak
PYTHONPATH=src python -m game_engine.cli make-dlc --version 1.0.1 --dlc-id skin-pack --out /tmp/dlc.json dlc.pak
PYTHONPATH=src python -m game_engine.cli ingest-crash --payload examples/ops/crash_payload.json --out-dir /tmp/crashes
```

## Existing authoring/validation commands

```bash
PYTHONPATH=src python -m game_engine.cli scene-new --scene-guid demo --out /tmp/scene.json
PYTHONPATH=src python -m game_engine.cli scene-add-entity --path /tmp/scene.json --entity-id player
PYTHONPATH=src python -m game_engine.cli asset-check-reimport --prev-source-hash a --prev-settings-hash b --source-path a.glb --source-hash a --settings-hash c
PYTHONPATH=src python -m game_engine.cli validate-plugin-manifest --path examples/minimal-game/plugin_manifest.json
PYTHONPATH=src python -m game_engine.cli validate-auth-token --path examples/minimal-game/auth_token.json
```


## Start a real local game project (scaffold -> run -> build)

```bash
PYTHONPATH=src python -m game_engine.cli new-game --project-dir /tmp/my_game --name my_game --version 0.1.0
PYTHONPATH=src python -m game_engine.cli run-game --project-dir /tmp/my_game --frames 120 --frame-time 16.6
PYTHONPATH=src python -m game_engine.cli build-game --project-dir /tmp/my_game --out-dir /tmp/my_game/dist --platform desktop
```

This gives you a runnable local game project folder (`game.project.json`, startup scene), then packages a build artifact you can iterate on.


## Playability upgrade (reach goal gameplay)

```bash
PYTHONPATH=src python -m game_engine.cli run-game --project-dir /tmp/my_game --frames 3 --frame-time 1000 --input-script examples/minimal-game/input_win_script.json
```

`run-game` output now includes `player_x`, `player_z`, `distance_to_goal`, `collected_coin`, `score`, and `won`.


## Runtime phase profiler + resource stats

`run-game` now reports per-frame phase timings and resource manager stats:
- `phase_input_ms`, `phase_gameplay_ms`, `phase_render_ms`
- `resources`, `resource_refs`


## Core/Pipeline completeness commands

```bash
PYTHONPATH=src python -m game_engine.cli pipeline-verify --manifest-a /tmp/a.json --manifest-b /tmp/b.json
PYTHONPATH=src python -m game_engine.cli patch-verify --base 1.0.0 --target 1.1.0
PYTHONPATH=src python -m game_engine.cli config-merge --base /tmp/config.json --override-json '{"difficulty":"hard"}'
PYTHONPATH=src python -m game_engine.cli event-smoke
PYTHONPATH=src python -m game_engine.cli job-smoke --value 9
```


## Performance benchmark plan

Run a baseline benchmark matrix (idle / win path / scale_100) and gate it:

```bash
PYTHONPATH=src scripts/benchmark.sh /tmp/ge_benchmark artifacts/bench
```

Manual commands:

```bash
PYTHONPATH=src python -m game_engine.cli benchmark-run --project-dir /tmp/ge_benchmark --out-json artifacts/bench/results.json --out-csv artifacts/bench/results.csv --win-script examples/minimal-game/input_win_script.json
PYTHONPATH=src python -m game_engine.cli benchmark-gate --results artifacts/bench/results.json --thresholds benchmarks/thresholds.json
```
