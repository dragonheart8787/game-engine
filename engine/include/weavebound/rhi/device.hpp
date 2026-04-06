#pragma once

#include <memory>

#include "weavebound/base/macros.hpp"
#include "weavebound/rhi/lit_demo_frame.hpp"
#include "weavebound/rhi/resources.hpp"
#include "weavebound/rhi/types.hpp"

namespace weavebound::platform {
class IWindow;
}

namespace weavebound::rg {
struct CompiledRenderGraph;
}

namespace weavebound::rhi {

struct DeviceDesc {
  Backend backend{Backend::Vulkan};
  bool enable_debug_layers{false};
  /** M2：啟用定向光陰影 + HDR 離屏 + tonemap／bloom 示範路徑（僅 Vulkan 實作）。 */
  bool enable_lit_demo{false};
  /** Win32：需為 `create_win32_window`；stub 無 HWND 時 Vulkan 建立會失敗。 */
  platform::IWindow* surface_target{nullptr};
};

/**
 * RHI 裝置；Renderer / Render Graph 僅透過此介面操作 GPU。
 * M1：Win64 + Vulkan SDK 時可建立 swapchain；否則回傳 nullptr。
 */
class IDevice : public NonCopyable {
 public:
  virtual ~IDevice();
  virtual Backend backend() const = 0;
  virtual bool is_valid() const = 0;

  /**
   * 清屏並提交至 swapchain（M1：Vulkan 實作；其餘後端預設 false）。
   * 色彩為線性 0–1；每幀呼叫一次以驅動畫面更新。
   * `render_graph` 非空且 `ok` 時，Vulkan 後端會先錄製對應 memory barriers 與 debug pass 標籤。
   */
  /**
   * `lit`：可傳 `LitDemoFrameParams`（ImGui `ImDrawData*`、示範時間、滑鼠相機等）；nullptr 表示預設（無
   * ImGui、時間 0、時間軌道相機）。
   */
  virtual bool clear_present_rgba(float r, float g, float b, float a,
                                  const rg::CompiledRenderGraph* render_graph = nullptr,
                                  const LitDemoFrameParams* lit = nullptr) {
    (void)r;
    (void)g;
    (void)b;
    (void)a;
    (void)render_graph;
    (void)lit;
    return false;
  }

  /** Vulkan/Metal 實作待補；預設 false。 */
  virtual bool create_buffer(const BufferDesc& desc, BufferHandle& out) {
    (void)desc;
    (void)out;
    return false;
  }

  /** 上傳至 device-local buffer（內部可用 staging）；預設 false。 */
  virtual bool upload_buffer(const BufferHandle& buffer, const void* data, std::size_t size,
                             std::size_t offset = 0) {
    (void)buffer;
    (void)data;
    (void)size;
    (void)offset;
    return false;
  }

  /** 最小 compute dispatch（M1）；預設 false。 */
  virtual bool dispatch_compute(std::uint32_t group_x, std::uint32_t group_y, std::uint32_t group_z) {
    (void)group_x;
    (void)group_y;
    (void)group_z;
    return false;
  }

  /** 建立 GPU 影像（device-local）；預設 false。 */
  virtual bool create_image(const ImageDesc& desc, ImageHandle& out) {
    (void)desc;
    (void)out;
    return false;
  }

  virtual void destroy_image(const ImageHandle& image) {
    (void)image;
  }
};

std::unique_ptr<IDevice> create_device(const DeviceDesc& desc);

}  // namespace weavebound::rhi
