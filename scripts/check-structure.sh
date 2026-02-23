#!/usr/bin/env bash
set -euo pipefail
required=(
  "engine/runtime/core/engine-loop.md"
  "editor/README.md"
  "pipeline/README.md"
  "services/README.md"
  "ecosystem/README.md"
  "docs/architecture/platform-overview.md"
)
for p in "${required[@]}"; do
  [[ -f "$p" ]] || { echo "Missing: $p"; exit 1; }
done
echo "Structure check passed"
