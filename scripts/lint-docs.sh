#!/usr/bin/env bash
set -euo pipefail

if [[ ! -f README.md ]]; then
  echo "README.md is required"
  exit 1
fi

if [[ ! -s README.md ]]; then
  echo "README.md must not be empty"
  exit 1
fi

if ! grep -qE '^# ' README.md; then
  echo "README.md must contain a top-level heading"
  exit 1
fi

echo "README structure looks good"
