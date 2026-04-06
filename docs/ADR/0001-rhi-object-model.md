# ADR 0001：RHI 物件模型與後端邊界

## 狀態

已採納（2026-04；隨實作演進可修訂「後續」小節）

## 情境

WeaveBound 需同時支援 Vulkan、Metal、D3D12（第二階段），且 Renderer／Render Graph 不得直連各 API。

## 決策

1. **控制代碼型別**：`BufferHandle` / `ImageHandle` / `SamplerHandle`（`rhi/resources.hpp`）作為跨後端不透明代碼；實際資源由後端實作持有。
2. **裝置入口**：`rhi::IDevice` 為建立 swapchain、提交幀、（未來）配置資源與 command 的統一入口；M1 以 Vulkan `clear_present_rgba` 驗證閉環。
3. **PSO / Command / Binding**：`GraphicsPsoDesc` / `ComputePsoDesc`、`ICommandBuffer` / `IFence`、`IDescriptorLayout` 先以**標頭契約**固定名稱與責任，避免各後端命名發散。
4. **後端優先序**：Vulkan → Metal → D3D12；新功能預設先落地 Vulkan，再以介面適配 Metal。

## 後果

- **優點**：上層可依型別與介面實作 Renderer，不依賴平台標頭。
- **代價**：需維護多後端轉換層與測試矩陣；Compute 與 descriptor 完整實作工時高。

## 後續（2026-04 對齊 Vulkan 產品化）

1. **Frame-in-flight**：交換鏈與 per-frame 命令緩衝／fence 由 `device_vulkan` 持有；上層以 `IDevice::clear_present_rgba` 等 API 提交，不在此 ADR 重複細節。
2. **影像 barrier 共用**：`rhi/vulkan/image_ops.{hpp,cpp}` 提供 `cmd_image_barrier`，Lit demo 與其他錄製路徑應優先呼叫，避免各處複製 `VkImageMemoryBarrier` 填寫邏輯。
3. **Layout 假設**：與 Render Graph 編譯器輸出之 `vk_old_image_layout`／`vk_new_image_layout`／`vk_aspect_mask` 對齊；深度資源使用 `DEPTH` aspect 與 `DEPTH_STENCIL_*` layout，不得混用 color-only barrier。

## 相關檔案

- `engine/include/weavebound/rhi/*.hpp`
- `engine/src/weavebound/rhi/vulkan/device_vulkan.cpp`
- `engine/include/weavebound/rhi/vulkan/image_ops.hpp`
