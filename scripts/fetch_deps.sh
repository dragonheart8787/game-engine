#!/usr/bin/env bash
set -euo pipefail

vendor_dir="third_party"
glm_tag="0.9.9.8"
json_tag="v3.11.3"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --vendor-dir)
      vendor_dir="$2"
      shift 2
      ;;
    --glm)
      glm_tag="$2"
      shift 2
      ;;
    --json)
      json_tag="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1" >&2
      echo "Usage: $0 --vendor-dir <path> --glm <tag> --json <tag>"
      exit 1
      ;;
  esac
done

mkdir -p "$vendor_dir"
if [[ ! -d "$vendor_dir/glm/.git" ]]; then
  rm -rf "$vendor_dir/glm"
  git clone --depth 1 --branch "$glm_tag" https://github.com/g-truc/glm.git "$vendor_dir/glm"
fi
if [[ ! -d "$vendor_dir/json/.git" ]]; then
  rm -rf "$vendor_dir/json"
  git clone --depth 1 --branch "$json_tag" https://github.com/nlohmann/json.git "$vendor_dir/json"
fi

echo "Dependencies fetched into $vendor_dir"
