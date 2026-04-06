# game-engine-platform

**產品規格（WeaveBound）：** 見 [docs/WEAVEBOUND_SPEC.md](docs/WEAVEBOUND_SPEC.md)（引擎定位、RHI/Render Graph/ECS/資產管線與階段路線圖）。

---

Dual-lane integration branch that preserves both:

- **Python lane:** `pyproject.toml`, `src/game_engine`, and `pytest`.
- **C++ lane:** `CMakeLists.txt`, `engine/`, `apps/`, `tools/`, `assets/`, and `ctest`.

## CI jobs

The CI workflow runs these jobs:

- `python-test`
- `cpp-build-linux`
- `windows-sanity`
- `android-stub`

## WeaveBound C++（Win64 Vulkan）

安裝 [Vulkan SDK](https://vulkan.lunarg.com/) 後，CMake 會偵測 `Vulkan::Vulkan` 並編譯 `device_vulkan.cpp`（swapchain、render pass、**三角形 pipeline**、`vkQueuePresentKHR`）。`glslc` 會編譯 `engine/shaders/*.vert|frag`；若找不到 `glslc`，可將預編譯的 `.spv` 放到 `engine/shaders/baked/`（見下方腳本）。`IDevice::clear_present_rgba`：清屏後繪製內建三角形。除錯：`WEAVEBOUND_VK_DEBUG=1`。Smoke：`WEAVEBOUND_SMOKE_FRAMES`（預設 120）。

### 一鍵下載／安裝相依（建議）

- **Windows**（winget + Vulkan 安裝程式下載 + pip）：

  ```powershell
  powershell -ExecutionPolicy Bypass -File scripts/bootstrap/download_deps_windows.ps1
  ```

  工作區上層目錄也可執行：`download_workspace_deps.ps1`（會轉呼叫上述腳本）。

- **Linux**（apt + pip）：

  ```bash
  bash scripts/bootstrap/download_deps_linux.sh
  ```

- **著色器 SPIR-V**（需已設定 `VULKAN_SDK` 或已安裝 `glslang-tools`）：

  ```powershell
  powershell -ExecutionPolicy Bypass -File scripts/bootstrap/compile_shaders.ps1
  ```

  ```bash
  bash scripts/bootstrap/compile_shaders.sh
  ```

  輸出至 `engine/shaders/baked/`，CMake 在無 `glslc` 時會改複製該目錄。

## Run checks locally

```bash
bash scripts/check-structure.sh
bash scripts/lint-docs.sh
pytest
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
bash scripts/build_android_stub.sh
```

## WeaveBound 原型發行建置（Windows）

- **目標程式**：設定並建置後執行 `build/<cfg>/weavebound_game_prototype.exe`（或專案中 CMake 定義的同等目標名稱）。
- **互動遊玩**：`weavebound_game_prototype.exe --play`（三段落戰役：到達終點兩次後第三段才結算勝利；標題列 HUD 顯示段落）。
- **音效**：Windows 下使用 `MessageBeep`（無音檔）；遊戲中／暫停／主選單按 **V** 切換開關。
- **驗收清單**：[docs/game/SHIP_CHECKLIST_WEAVEBOUND.md](docs/game/SHIP_CHECKLIST_WEAVEBOUND.md)。

建置範例（已安裝 CMake、Visual Studio 或 Ninja 工具鏈）：

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
