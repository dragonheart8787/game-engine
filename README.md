# game-engine-platform（WeaveBound）

**遠端儲存庫：** [github.com/dragonheart8787/game-engine](https://github.com/dragonheart8787/game-engine)

**產品規格（WeaveBound）：** 見 [docs/WEAVEBOUND_SPEC.md](docs/WEAVEBOUND_SPEC.md)（引擎定位、RHI/Render Graph/ECS/資產管線與階段路線圖）。

---

## 雙線整合（Dual-lane）

- **Python lane：** `pyproject.toml`、`src/game_engine`、`pytest`。
- **C++ lane：** `CMakeLists.txt`、`engine/`、`apps/`、`tools/`、`assets/`、`ctest`。

## CI jobs

Workflow 包含（依 `.github/workflows` 為準）：

- `python-test`
- `cpp-build-linux`
- `windows-sanity`
- `android-stub`

## WeaveBound C++（Win64，可選 Vulkan）

安裝 [Vulkan SDK](https://vulkan.lunarg.com/) 後，CMake 會偵測 `Vulkan::Vulkan` 並編譯 `device_vulkan.cpp`（swapchain、render pass、**三角形 pipeline**、`vkQueuePresentKHR`）。`glslc` 會編譯 `engine/shaders/*.vert|frag`；若找不到 `glslc`，可將預編譯的 `.spv` 放到 `engine/shaders/baked/`（見下方腳本）。`IDevice::clear_present_rgba`：清屏後可繪製內建幾何／lit 示範路徑。除錯：`WEAVEBOUND_VK_DEBUG=1`。Smoke：`WEAVEBOUND_SMOKE_FRAMES`（預設 120）。

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

Windows 僅跑 C++ 與原型測試時，可在 `build` 目錄：

```powershell
ctest -C Debug -R weavebound_game_prototype --output-on-failure
```

---

## WeaveBound 原型（`weavebound_game_prototype`）— Windows 建置與遊玩

### 建置

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

可執行檔路徑：`build\<Configuration>\weavebound_game_prototype.exe`（Debug 則為 `build\Debug\...`）。

### 建議執行目錄

請在 **與 exe 相同目錄** 下啟動（以便載入同目錄之 `ability_slice_v0.json` 等）：

```powershell
cd build\Debug
.\weavebound_game_prototype.exe --play
```

### 啟動參數一覽

| 參數 | 說明 |
|------|------|
| `--play` | **預設交付用**：**不保證 Vulkan**；**保證 2D 戰場頂視**（約上方 2/3：網格、終點／玩家／敵人、圖例），下方為操作說明；標題列另有 HUD。 |
| `--play-vk` / `--play-3d` | 嘗試 **Vulkan 3D + ImGui**；若裝置或呈現失敗，會改以與 `--play` 相同的 **2D 戰場** 顯示。 |
| `--play-2d` | 與 `--play` 相同（明確表示不要走 3D 初始化）。 |
| `--ci-gameover`、`--save-smoke`、`--ability-smoke` 等 | 自動化／smoke 測試用（見 `main.cpp` 與 CTest）。 |

### 玩法與系統摘要

- **戰役**：三段落—到達終點兩次後第三段才結算勝利。
- **存檔／設定**：slot 與原型設定 JSON（暫停選單可寫回設定，依建置與模式而定）。
- **CTest**：原型相關測試會 `chdir` 至 exe 目錄再執行，避免找不到資料檔。
- **Eidrix**：若同層有 `../eidrix_mvp`，可 `cmake --build build --target eidrix_pytest` 跑對方 pytest。
- **音效**：Windows 下使用 `MessageBeep`（無音檔）；遊戲中／暫停／主選單按 **V** 切換開關。
- **驗收清單**：[docs/game/SHIP_CHECKLIST_WEAVEBOUND.md](docs/game/SHIP_CHECKLIST_WEAVEBOUND.md)。

### 已知限制（原型階段）

- **2D 路徑** 目前為 GDI 每幀重繪，在部分環境可能感到**閃爍**；後續可改雙緩衝或縮小 invalidate 範圍。
- **3D** 為引擎 **lit 示範 + ImGui**，非成品關卡美術。
- **Loading** 狀態仍偏占位，真實場景載入待擴充。

---

## 同步到 GitHub

```bash
git remote add origin https://github.com/dragonheart8787/game-engine.git   # 若尚未設定
git add -A
git commit -m "同步原型：2D 保證畫面、Vulkan 可選、平台與測試修正"
git push origin main
```

（若 `origin` 已存在且為上述 URL，只需 `git push`。）
