#pragma once

#include <cstdint>
#include <memory>

#include <vulkan/vulkan.h>

#include "weavebound/rhi/lit_demo_frame.hpp"

namespace weavebound::rhi {

/** Lit demo（陰影 / HDR / 簡化 bloom / ImGui）初始化上下文；僅 Vulkan 後端於內部填入。 */
struct LitDemoInitCtx {
  VkInstance instance{};
  VkDevice device{};
  VkPhysicalDevice physical{};
  VkQueue graphics_queue{};
  std::uint32_t graphics_queue_family{};
  VkCommandPool command_pool{};
  VkRenderPass swapchain_render_pass{};
  VkFormat swapchain_format{};
  VkExtent2D swapchain_extent{};
  std::uint32_t swapchain_image_count{};
  /** Win32 HWND，供 Dear ImGui Win32 後端；可為 nullptr 則不啟用 ImGui。 */
  void* native_window_handle{};
};

/**
 * M2 垂直切片：定向光陰影、HDR 離屏、全螢幕 tonemap+bloom、可選 ImGui 疊加。
 * 由 device_vulkan 持有；record 須在已 begin 的一級 command buffer 內、且尚未開始 swapchain render pass。
 */
class LitDemoRecorder {
 public:
  LitDemoRecorder();
  ~LitDemoRecorder();

  LitDemoRecorder(const LitDemoRecorder&) = delete;
  LitDemoRecorder& operator=(const LitDemoRecorder&) = delete;

  bool init(const LitDemoInitCtx& ctx);
  void shutdown();

  bool is_ready() const;

  void on_swapchain_resized(VkExtent2D extent, VkFormat swapchain_format, VkRenderPass swapchain_rp,
                            std::uint32_t image_count);

  /** 錄製陰影 + HDR + swapchain 合成（含 ImGui）。不呼叫 vkBegin/end CommandBuffer。 */
  bool record(VkCommandBuffer cmd, VkFramebuffer swapchain_fb, VkExtent2D extent, const float clear_rgba[4],
              const LitDemoFrameParams* frame);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace weavebound::rhi
