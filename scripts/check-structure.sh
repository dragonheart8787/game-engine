```bash
#!/usr/bin/env bash
set -euo pipefail

required=(
  "pyproject.toml"
  "src/game_engine/cli.py"
  "src/game_engine/runtime/contracts.py"
  "src/game_engine/runtime/engine.py"
  "src/game_engine/ecosystem/contracts.py"
  "tests/test_runtime_mvp.py"
  "CMakeLists.txt"
  "engine/CMakeLists.txt"
  "apps/demo_thirdperson/CMakeLists.txt"
)

for p in "${required[@]}"; do
  [[ -f "$p" ]] || { echo "Missing: $p"; exit 1; }
done

echo "Structure check passed"