#!/usr/bin/env bash
set -euo pipefail

# Ensure SDL usage does not leak into this integration baseline.
if grep -RInE '\bSDL[0-9_]*\b' engine apps 2>/dev/null; then
  echo "SDL references found in engine/apps; keep SDL integration isolated from this baseline branch"
  exit 1
fi

if grep -InE '\bSDL[0-9_]*\b' CMakeLists.txt 2>/dev/null; then
  echo "SDL references found in CMakeLists.txt; keep SDL integration isolated from this baseline branch"
  exit 1
fi

echo "SDL isolation check passed"
