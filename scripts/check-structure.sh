#!/usr/bin/env bash
set -euo pipefail

required_python=(
  "pyproject.toml"
  "src/game_engine/runtime/contracts.py"
  "src/game_engine/editor/contracts.py"
  "src/game_engine/pipeline/schema.py"
  "src/game_engine/services/contracts.py"
  "src/game_engine/ecosystem/contracts.py"
  "src/game_engine/ops/crash.py"
  "src/game_engine/governance/compliance.py"
)

required_cpp=(
  "CMakeLists.txt"
  "engine/include/game_engine/core.hpp"
  "engine/src/core.cpp"
  "apps/smoke/main.cpp"
  "tools/README.md"
  "assets/.keep"
  "scripts/check_sdl_isolation.sh"
)

for p in "${required_python[@]}"; do
  [[ -f "$p" ]] || { echo "Missing Python lane artifact: $p"; exit 1; }
done

for p in "${required_cpp[@]}"; do
  [[ -f "$p" ]] || { echo "Missing C++ lane artifact: $p"; exit 1; }
done

echo "Structure check passed (Python + C++ lanes)"
