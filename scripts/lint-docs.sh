#!/usr/bin/env bash
set -euo pipefail
md_count=$(rg --files -g '*.md' | wc -l)
if [[ "$md_count" -gt 1 ]]; then
  echo "Too many markdown files left: $md_count (expected only README.md)"
  exit 1
fi
echo "Markdown-to-code migration check passed"
