#pragma once

#include "weavebound/render_graph/compiled.hpp"
#include "weavebound/rhi/device.hpp"

namespace weavebound::rg {

/**
 * Render Graph 與 RHI 幀提交之薄包裝：M1 實際錄製在 `IDevice::clear_present_rgba(..., crg)`（Vulkan）。
 */
class RenderGraphExecutor {
 public:
  explicit RenderGraphExecutor(rhi::IDevice& device) : device_(device) {}

  bool clear_present(float r, float g, float b, float a, const CompiledRenderGraph* crg) {
    return device_.clear_present_rgba(r, g, b, a, crg);
  }

 private:
  rhi::IDevice& device_;
};

}  // namespace weavebound::rg
