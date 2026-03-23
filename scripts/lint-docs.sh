#!/usr/bin/env bash
set -euo pipefail

[[ -f README.md ]] || { echo "README.md missing"; exit 1; }

echo "Docs lint passed"