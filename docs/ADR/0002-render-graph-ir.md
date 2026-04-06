# ADR 0002：Render Graph IR 與編譯管線

## 狀態

已採納（2026-04）

## 情境

手工 barrier 與 resource state 在 Vulkan/Metal 上易錯；需要可測試的 IR 與確定性編譯輸出（含除錯 JSON）。

## 決策

1. **IR 結構**：`ResourceNode`（id、name、transient、**surface**：`ResourceSurfaceKind` 區分 **Color** / **Depth**，影響編譯器推斷之 layout 與 aspect）、`PassNode`（id、name、kind、reads、writes）定義於 `render_graph/ir.hpp`。
2. **建置 API**：`RenderGraphBuilder` 負責加入 resource/pass 並宣告讀寫；不於 builder 階段推導 barrier。
3. **編譯**：`compile(builder)` 依「最後寫入者」規則建立 pass 間依賴邊，**拓撲排序**；環偵測時 `ok=false` 並附錯誤字串。
4. **Barrier**：`BarrierRecord` 含 `state_before`／`state_after`（`rhi::ResourceState`）與 Vulkan 對齊之 `vk_*_mask`、`vk_old_image_layout`／`vk_new_image_layout`、`vk_aspect_mask`；編譯器依 producer／consumer `PassKind` 與資源 **surface** 推斷（如 color attachment → SRV、depth attachment → read-only depth；note 分別為 `color_attachment_to_srv`、`depth_attachment_to_srv`）。`compile()` 失敗時 `ok=false`。
5. **執行**：Vulkan 於 `IDevice::clear_present_rgba(..., crg)` 內、進入 swapchain render pass 前錄製 `vkCmdPipelineBarrier`（memory barrier）與（驗證層開啟時）`vkCmdBegin/EndDebugUtilsLabelEXT` pass 名稱。
6. **除錯**：`dump_json()` 含頂層 **`"schema":1`**（版本化契約）、resources 內 **`surface`**、barriers 內 **`vk_aspect_mask`** 與其餘欄位，供 CI 快照比對。

## 後果

- **優點**：後續 PBR 多 pass、async compute、present transition 可在同一 IR 上擴充。
- **代價**：需補 lifetime/alias、transient attachment 與實際 GPU barrier 映射；JSON 格式可能版本化。

## 相關檔案

- `engine/include/weavebound/render_graph/{ir,builder,compiled,graph,executor}.hpp`
- `engine/src/weavebound/render_graph/{builder,compiler}.cpp`
