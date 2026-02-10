#!/usr/bin/env bash
set -euo pipefail

missing=0

if [[ ! -f third_party/glm/glm/glm.hpp && ! -f /usr/include/glm/glm.hpp ]]; then
  echo "Missing glm: expected third_party/glm/glm/glm.hpp or system glm package"
  missing=1
fi

if [[ ! -f third_party/json/include/nlohmann/json.hpp && ! -f /usr/include/nlohmann/json.hpp ]]; then
  echo "Missing nlohmann_json: expected third_party/json/include/nlohmann/json.hpp or system package"
  missing=1
fi

if [[ $missing -ne 0 ]]; then
  echo "Vendor/system dependency completeness check failed"
  exit 1
fi

echo "Vendor/system dependency completeness check passed"
