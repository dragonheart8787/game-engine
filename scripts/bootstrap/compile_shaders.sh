#!/usr/bin/env bash
# 使用系統 glslangValidator（apt install glslang-tools）或 VULKAN_SDK/Bin/glslc 編譯著色器。
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/engine/shaders"
OUT="$SRC/baked"
mkdir -p "$OUT"

compile_pair() {
  local vert="$1" frag="$2"
  if command -v glslc >/dev/null 2>&1; then
    glslc -o "$OUT/${vert}.spv" "$SRC/$vert"
    glslc -o "$OUT/${frag}.spv" "$SRC/$frag"
  elif [[ -n "${VULKAN_SDK:-}" && -x "${VULKAN_SDK}/bin/glslc" ]]; then
    "${VULKAN_SDK}/bin/glslc" -o "$OUT/${vert}.spv" "$SRC/$vert"
    "${VULKAN_SDK}/bin/glslc" -o "$OUT/${frag}.spv" "$SRC/$frag"
  elif command -v glslangValidator >/dev/null 2>&1; then
    glslangValidator -V "$SRC/$vert" -o "$OUT/${vert}.spv"
    glslangValidator -V "$SRC/$frag" -o "$OUT/${frag}.spv"
  else
    echo "Install glslc (Vulkan SDK) or glslang-tools (glslangValidator)." >&2
    exit 1
  fi
}

compile_pair triangle.vert triangle.frag

compile_comp() {
  local comp="$1"
  if command -v glslc >/dev/null 2>&1; then
    glslc -o "$OUT/${comp}.spv" "$SRC/$comp"
  elif [[ -n "${VULKAN_SDK:-}" && -x "${VULKAN_SDK}/bin/glslc" ]]; then
    "${VULKAN_SDK}/bin/glslc" -o "$OUT/${comp}.spv" "$SRC/$comp"
  elif command -v glslangValidator >/dev/null 2>&1; then
    glslangValidator -V "$SRC/$comp" -o "$OUT/${comp}.spv"
  else
    echo "Install glslc (Vulkan SDK) or glslang-tools (glslangValidator)." >&2
    exit 1
  fi
}

compile_comp trivial.comp
echo "OK -> $OUT"
