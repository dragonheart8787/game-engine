# game-engine

這個 repo 是 12 週專用引擎骨架計畫的起始版，遵守指定的模組邊界與資料格式。
目前提供可編譯的 Demo（第三人稱移動 + 場景渲染）與最小的世界狀態/能力/故事系統骨架。

## ✅ 已完成

- **SDL2 + OpenGL ES 3.0** 視窗與渲染管線（PC）
- 第三人稱 Demo：角色移動 / 跳躍 / 衝刺 / 基本 Ability 觸發
- WorldState/WorldDelta JSON 解析與 Hash
- AbilityGraph v0 解析 + 事件輸出
- Story Director/Identity Override 最小 runtime
- 工具與測試骨架：asset_packer stub、worldstate_tests

> 選擇 OpenGL ES 3.0：符合跨 PC/Android 的同一條渲染 API 要求，降低平台分歧。

## 🧱 目錄結構（嚴格版）

```
/engine
  /core /math /ecs /world /render /vfx /input /audio /physics /ai /net /story /procgen /ability /tools /debug /platform
/apps
  /demo_thirdperson
/tools
  /asset_packer
/tests
  /worldstate_tests
/assets
  /scenes /abilities /story /shaders /models /textures /audio
/scripts
/docs
```

## 🛠️ 建置與執行（PC）

### 需求
- CMake 3.16+
- C++17 編譯器
- SDL2
- OpenGL ES 3.x dev 套件（Linux 下通常是 `libgles2-mesa-dev`）

### Ubuntu/Debian
```
sudo apt-get update
sudo apt-get install -y build-essential cmake libsdl2-dev libgles2-mesa-dev
```

### macOS (Homebrew)
```
brew install cmake sdl2
```

### Windows (vcpkg)
```
vcpkg install sdl2
```
```
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

### Build
```
cmake -S . -B build
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

Android 目前提供 build stub 與抽象層，尚未完成完整 pipeline。

## ✅ 下一步（必備措施）

1. 完成 WorldState deterministic replay + input recording/replay
2. 實作 AbilityGraph 對 VFX/Combat 的最小橋接
3. Story beats 驗證與 WorldDelta 產出
4. 場景 JSON hot reload 與 shader reload pipeline

更多規格細節請見 `docs/ARCH_SPEC.md`。
