# ADR 0003：ECS 儲存模型（Hybrid / C++ 執行時）

## 狀態

提案（待 M3 落地；目前僅 `ecs/scene_types.hpp` 占位）

## 情境

遊戲邏輯需要可擴充的 component 模型與（最終）多執行緒 system；Python 層已有極簡 registry，但執行時核心應以 C++ 為準。

## 決策（目標架構）

1. **EntityId**：32-bit 單調或世代式（避免 stale reference）；與 `scene_types.hpp` 對齊後再定稿。
2. **SoA**：依 component type 分欄儲存；Transform / MeshRenderer 等高頻資料優先 SoA。
3. **Scheduler**：先單執行緒 system 排序；再引入 job 依 DAG 或階段並行（與 `platform/job_system.hpp` 整合）。
4. **與場景**：Prefab、序列化、版本升級走獨立管線（與 `scene_tool`、資產 bundle 協同）。

## 後果

- **優點**：效能與快取友善；與 Render Graph「每幀編譯」可解耦。
- **代價**：遷移 Python 邏輯需明確邊界；需寫入大量單元測試與 SoA 遷移工具。

## 相關檔案

- `engine/include/weavebound/ecs/scene_types.hpp`
- （未來）`engine/include/weavebound/ecs/registry.hpp` 等
