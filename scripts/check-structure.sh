#!/usr/bin/env bash
set -euo pipefail
required=(
  "pyproject.toml"
  "src/game_engine/runtime/contracts.py"
  "src/game_engine/editor/contracts.py"
  "src/game_engine/pipeline/schema.py"
  "src/game_engine/services/contracts.py"
  "src/game_engine/ecosystem/contracts.py"
  "src/game_engine/ops/crash.py"
  "src/game_engine/governance/compliance.py"
)
for p in "${required[@]}"; do
  [[ -f "$p" ]] || { echo "Missing: $p"; exit 1; }
done
echo "Structure check passed"
