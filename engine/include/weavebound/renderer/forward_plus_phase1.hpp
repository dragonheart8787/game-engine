#pragma once

#include "weavebound/renderer/cull.hpp"

#include <cstddef>

namespace weavebound::ecs {
class World;
}

namespace weavebound::renderer {

/**
 * Phase 1 Forward+ 渲染器契約（規格 1.4）。
 * 完整 PBR／陰影／bloom 等在 RHI 管線擴充後逐步替換實作。
 */
class IForwardPlusPhase1Renderer {
 public:
  virtual ~IForwardPlusPhase1Renderer() = default;
  virtual void render_frame() = 0;
};

/**
 * CPU 端：以 ECS 中第一個相機元件建 view-proj，對具 mesh_renderer + aabb 的實體做視錐剔除並統計。
 * GPU 幀仍由 `IDevice::clear_present_rgba` 驅動。
 */
class ForwardPlusPhase1 final : public IForwardPlusPhase1Renderer {
 public:
  ForwardPlusPhase1(ecs::World& world, int viewport_w_px, int viewport_h_px)
      : world_(world), viewport_w_(viewport_w_px), viewport_h_(viewport_h_px) {}

  void set_viewport_pixels(int w, int h) {
    viewport_w_ = w;
    viewport_h_ = h;
  }

  void render_frame() override;

  std::size_t last_visible_draws() const { return visible_; }
  std::size_t last_total_meshes() const { return total_; }

 private:
  ecs::World& world_;
  int viewport_w_{1280};
  int viewport_h_{720};
  std::size_t visible_{0};
  std::size_t total_{0};
};

using ForwardPlusPhase1CullStub = ForwardPlusPhase1;

}  // namespace weavebound::renderer
