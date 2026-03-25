#!/usr/bin/env bash
set -euo pipefail

# Use tracked files so local/untracked docs do not break CI unexpectedly.
md_files=$(git ls-files '*.md')
md_count=$(printf '%s\n' "$md_files" | sed '/^$/d' | wc -l)

if [[ "$md_count" -gt 1 ]]; then
  echo "Too many markdown files left: $md_count (expected only README.md)"
  exit 1
fi

if [[ "$md_files" != "README.md" ]]; then
  echo "Unexpected markdown file set: ${md_files:-<none>} (expected only README.md)"
  exit 1
fi

if ! grep -q '^## Actionable Roadmap$' README.md; then
  echo 'README.md missing required section: Actionable Roadmap'
  exit 1
fi

echo "Markdown-to-code migration check passed"
