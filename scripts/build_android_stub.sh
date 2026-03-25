#!/usr/bin/env bash
set -euo pipefail

echo "[android-stub] Starting stubbed Android build flow"

for path in CMakeLists.txt engine/src/core.cpp apps/smoke/main.cpp; do
  if [[ ! -f "$path" ]]; then
    echo "[android-stub] missing required file: $path"
    exit 1
  fi
done

echo "[android-stub] Android toolchain not configured in CI; stub passed"
