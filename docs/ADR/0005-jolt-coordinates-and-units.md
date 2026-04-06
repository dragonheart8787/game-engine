# ADR 0005：Jolt 整合 — 座標、單位與射線語意

## 狀態

已採納（P0：`weavebound_physics` + `physics/jolt_world.cpp`）

## 情境

需在 Win64／MSVC 下以 **FetchContent** 引入 [JoltPhysics](https://github.com/jrouwe/JoltPhysics)，並與現有 `Application::fixed_update`、lit demo 假設的 **Y-up** 世界對齊；同時避免與主工程 **/MD** 執行階段函式庫衝突。

## 決策

1. **版本**：CMake 固定 `GIT_TAG v5.2.0`、`SOURCE_SUBDIR Build`；`WEAVEBOUND_WITH_JOLT` 預設 `ON`，關閉時 `WEAVEBOUND_WITH_JOLT=0` 且仍走 `integrations::physics_step_minimal` 占位。
2. **MSVC**：`USE_STATIC_MSVC_RUNTIME_LIBRARY=OFF`（與主專案動態 CRT 一致）；`ENABLE_ALL_WARNINGS=OFF`；對 `Jolt` 目標 `DISABLE_PRECOMPILE_HEADERS=ON`（避免與父專案 `/utf-8` + PCH 在部分環境觸發 C4828／MSB8084）。
3. **座標**：沿用 Jolt 預設與 HelloWorld 範例之 **右手、Y-up**；本波測試場景為原點附近地板 + 球，與 [lit_mesh.vert](../../engine/shaders/lit_mesh.vert) 所用視角假設一致（後續若改 Z-up 須單獨 ADR）。
4. **射線**：`RRayCast` 之 `mDirection` 同時為**方向與掃描長度**（見 Jolt `RayCast.h`）；單位向量僅掃 1 單位，向下探測地板須給足長度（例如 `(0,-32,0)`）。
5. **圖層**：採 HelloWorld 風格之 `ObjectLayer`／`BroadPhaseLayer` 與三種 filter；之後遊戲可改為表驅動。

## 後果

- **優點**：真實 broad/narrow phase、raycast、與 CI 可複製的 `physics_smoke`。
- **代價**：首次 Fetch／編譯 Jolt 體積大；離線建置需關閉 `WEAVEBOUND_WITH_JOLT` 或預先快取 `_deps`。

## 相關檔案

- `engine/include/weavebound/physics/jolt_world.hpp`
- `engine/src/weavebound/physics/jolt_world.cpp`
- `CMakeLists.txt`（`WEAVEBOUND_WITH_JOLT`、`USE_STATIC_MSVC_RUNTIME_LIBRARY`）
