#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="${1:-/tmp/ge_benchmark_project}"
ARTIFACT_DIR="${2:-artifacts/bench}"

mkdir -p "$ARTIFACT_DIR"

PYTHONPATH=src python -m game_engine.cli new-game --project-dir "$PROJECT_DIR" --name benchmark_game --version 0.1.0 >/dev/null

PYTHONPATH=src python -m game_engine.cli benchmark-run \
  --project-dir "$PROJECT_DIR" \
  --out-json "$ARTIFACT_DIR/results.json" \
  --out-csv "$ARTIFACT_DIR/results.csv" \
  --win-script examples/minimal-game/input_win_script.json

PYTHONPATH=src python -m game_engine.cli benchmark-gate \
  --results "$ARTIFACT_DIR/results.json" \
  --thresholds benchmarks/thresholds.json

echo "Benchmark completed: $ARTIFACT_DIR/results.json"
