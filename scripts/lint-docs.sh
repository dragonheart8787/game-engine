#!/usr/bin/env bash
set -euo pipefail
count=$(rg --files docs engine editor pipeline services ecosystem | wc -l)
if [[ "$count" -lt 20 ]]; then
  echo "Documentation footprint too small: $count"
  exit 1
fi
echo "Docs lint passed ($count files)"
