# game-engine

這個專案已經開始實作「可執行的引擎核心」，目前包含遊戲迴圈、輸入、時間、簡易 ECS-lite 世界、以及可見的渲染輸出（移動方塊）。

## ✅ 目前已具備（可用基礎）

- 遊戲迴圈 + 變動時間步進（Time）
- 輸入系統（鍵盤 + Quit）
- ECS-lite 世界（Entity + Transform + Velocity）
- SDL2 渲染（清畫面 + 方塊渲染）
- 可移動的 player entity（WASD / 方向鍵）

## 🧱 專案結構

```
src/
  main.cpp
  engine/
    Components.h
    Engine.h / Engine.cpp
    Input.h / Input.cpp
    Renderer.h / Renderer.cpp
    Time.h / Time.cpp
    World.h / World.cpp
```

## 🛠️ 建置與執行

### 需求
- CMake 3.16+
- C++17 編譯器
- SDL2

### 建置

```
cmake -S . -B build
cmake --build build
```

### 執行

```
./build/game_engine
```

**操作方式**
- 移動：WASD 或 方向鍵
- 退出：視窗關閉

> 如果 SDL2 沒安裝，請先在你的系統上安裝對應套件。

## ✅ 下一步目標（必備措施）

1. 加入 `Camera` / `Transform` 階層
2. 加入簡單 `AssetManager`（texture / mesh / shader）
3. 加入 `Scene` 載入（JSON + hot reload）
4. 加入最小碰撞（AABB / sphere）
5. 做出第一個可玩的房間 + 互動物件

## 🧭 核心方向（避免失控）

任何功能如果不能在 2 週內增加可玩的內容，就不做。

---

想繼續擴展的話，請提供你遊戲的 **類型 / 平台 / 核心玩法**，我可以直接幫你制定下一個實作階段。
