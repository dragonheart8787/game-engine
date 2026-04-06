#!/usr/bin/env bash
# Debian/Ubuntu：編譯與 Vulkan 驗證層、glslang、Python 相依。
set -euo pipefail

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "此腳本僅適用 Linux。" >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GAME_ENGINE_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
WORKSPACE_ROOT="$(cd "$GAME_ENGINE_ROOT/.." && pwd)"

echo "==> apt: 建置工具、Vulkan、glslang、Python"
sudo apt-get update -y
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  python3 \
  python3-pip \
  python3-venv \
  libvulkan1 \
  vulkan-tools \
  libvulkan-dev \
  mesa-vulkan-drivers \
  glslang-tools

echo "==> pip: game-engine"
python3 -m pip install --user --upgrade pip
python3 -m pip install --user -e "$GAME_ENGINE_ROOT"
python3 -m pip install --user pytest

if [[ -f "$WORKSPACE_ROOT/eidrix_mvp/requirements.txt" ]]; then
  echo "==> pip: eidrix_mvp"
  python3 -m pip install --user -r "$WORKSPACE_ROOT/eidrix_mvp/requirements.txt" || true
fi

echo "==> 著色器（選用，產生 baked SPIR-V）"
if bash "$SCRIPT_DIR/compile_shaders.sh"; then
  echo "baked OK"
else
  echo "著色器編譯略過（Linux 目前 C++ 未啟用 device_vulkan）"
fi

echo "完成。建置: cd \"$GAME_ENGINE_ROOT\" && cmake -S . -B build && cmake --build build"
