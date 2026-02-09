# game-engine

這個專案已經正式開始「可執行」的引擎骨架實作，目標是能夠快速跑起視窗、輸入、遊戲迴圈與基本渲染。

## ✅ 目前已具備

- 基本遊戲迴圈與時間步進
- 視窗建立與輸入事件處理
- SDL2 渲染器與每幀清畫面

## 🧱 專案結構

```
src/
  main.cpp
  engine/
    Engine.h / Engine.cpp
    Renderer.h / Renderer.cpp
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

> 如果 SDL2 沒安裝，請先在你的系統上安裝對應套件。

## ✅ 下一步目標（可落地）

1. 加入基本 `Transform` 與 `Camera`
2. 加入簡單 mesh 資源與 shader 管理
3. 放入一個可移動的 player controller
4. 做出「第一個可玩房間」

## 🧭 核心方向（避免失控）

任何功能如果不能在 2 週內增加可玩的內容，就不做。

---

想繼續擴展的話，請提供你遊戲的 **類型 / 平台 / 核心玩法**，我可以直接幫你制定下一個實作階段。
