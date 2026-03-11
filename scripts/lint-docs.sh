#!/usr/bin/env bash
set -euo pipefail
if [[ ! -f README.md ]]; then
  echo "README.md missing"
  exit 1
fi

required_sections=(
  "Actionable Roadmap"
  "Quick start"
  "Demo run"
)
for section in "${required_sections[@]}"; do
  if ! rg -q "$section" README.md; then
    echo "README.md missing required section: $section"
    exit 1
  fi
done

required_commands=(
  "check-structure.sh"
  "pytest"
  "run-demo"
)
for cmd in "${required_commands[@]}"; do
  if ! rg -q "$cmd" README.md; then
    echo "README.md missing required command reference: $cmd"
    exit 1
  fi
done

echo "Docs lint passed"
