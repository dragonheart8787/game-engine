#!/usr/bin/env bash
set -euo pipefail

violations=$(rg -n "#include <SDL" engine apps tests tools | rg -v "engine/core/Platform.cpp|engine/core/Platform.h" || true)
if [[ -n "$violations" ]]; then
  echo "SDL include isolation violated:"
  echo "$violations"
  exit 1
fi

echo "SDL include isolation check passed"
