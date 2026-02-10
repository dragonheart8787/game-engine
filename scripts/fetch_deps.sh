#!/usr/bin/env bash
set -euo pipefail

mkdir -p third_party
if [[ ! -d third_party/glm ]]; then
  git clone --depth 1 --branch 0.9.9.8 https://github.com/g-truc/glm.git third_party/glm
fi
if [[ ! -d third_party/json ]]; then
  git clone --depth 1 --branch v3.11.3 https://github.com/nlohmann/json.git third_party/json
fi

echo "Dependencies fetched into third_party/"
