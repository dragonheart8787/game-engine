# game-engine

這個 repo 是 12 週專用引擎骨架計畫的起始版，遵守指定的模組邊界與資料格式。
目前提供可編譯的 Demo（第三人稱移動 + 場景渲染）與最小的世界狀態/能力/故事系統骨架。

## ✅ 已完成

- **SDL2 + OpenGL ES 3.0** 視窗與渲染管線（PC）
- 第三人稱 Demo：角色移動 / 跳躍 / 衝刺 / 基本 Ability 觸發
- WorldState/WorldDelta JSON 解析與 Hash
- AbilityGraph v0 解析 + 事件輸出
- Story Director/Identity Override 最小 runtime
- 工具與測試骨架：asset_packer、validate_assets、replay_diff、worldstate_tests

> 選擇 OpenGL ES 3.0：符合跨 PC/Android 的同一條渲染 API 要求，降低平台分歧。

## 🧱 目錄結構（嚴格版）

```
/engine
  /core /math /ecs /world /render /vfx /input /audio /physics /ai /net /story /procgen /ability /tools /debug /platform
/apps
  /demo_thirdperson
/tools
  /asset_packer /validate_assets /replay_diff
/tests
  /worldstate_tests
/assets
  /scenes /abilities /story /shaders /models /textures /audio /config
/scripts
/docs
/third_party
```

## 🛠️ 建置與執行（PC）

### 需求
- CMake 3.16+
- C++17 編譯器
- SDL2
- OpenGL ES 3.x dev 套件（Linux 下通常是 `libgles2-mesa-dev`）
- glm
- nlohmann_json

### Ubuntu/Debian（離線友善）
```
sudo apt-get update
sudo apt-get install -y build-essential cmake libsdl2-dev libgles2-mesa-dev libglm-dev nlohmann-json3-dev
```

### Build（離線模式）
```
cmake -S . -B build -DENGINE_VENDOR_DEPS=ON
cmake --build build
ctest --test-dir build
```

### Build（線上模式，允許 FetchContent）
```
cmake -S . -B build -DENGINE_VENDOR_DEPS=OFF
cmake --build build
```

### Run Demo
```
./build/apps/demo_thirdperson/demo_thirdperson
```

### Demo 控制
- Move: WASD
- Look: 方向鍵
- Jump: Space
- Dash: Left Shift
- Cast Ability1: J
- Trigger Story A/B: T / Y
- Toggle Debug: L

## 🔁 App 迴圈順序（固定）
Platform::PollEvents → InputSystem::BeginFrame → Game::Update → Renderer::BeginFrame → Game::Render → Renderer::EndFrame → Platform::Present

## 🤖 Android (Stub)
- Build-only stub（CI 不跑 runtime）
- NDK 版本預設：`r26d`
- 觸控映射設定檔：`assets/config/touch_mapping.json`

## 🔒 Determinism Guardrails
- 固定 tick 計時：遊戲邏輯使用 fixed update。
- RNG 單一入口：`engine::math::DeterministicRng`（seed + streamId）。
- Input recording 為 versioned 格式（`version` + `seed` + `frames`）。
- WorldDelta 套用加入 validation / journaling / rollback。
- Golden hash 測試已加入（worldstate_tests）。

## 🧪 額外工具
- `tools/validate_assets`：檢查 Ability/Story/WorldDelta 語義錯誤。
- `tools/replay_diff`：輸出 replay 第一個 divergence tick。
- `scripts/check_sdl_isolation.sh`：檢查 SDL include 只存在於 platform 模組。
- `scripts/fetch_deps.sh`：可選線上抓 third_party 依賴。

更多規格細節請見 `docs/ARCH_SPEC.md`。
