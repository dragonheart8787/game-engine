#pragma once

#include <functional>
#include <memory>

#include "weavebound/ecs/registry.hpp"
#include "weavebound/platform/clock.hpp"
#include "weavebound/platform/window.hpp"
#include "weavebound/rhi/device.hpp"

namespace weavebound::engine {

/** 應用程式層：視窗 + 時鐘 + RHI + ECS World（單機主循環入口）。 */
struct ApplicationConfig {
  int width_px = 1280;
  int height_px = 720;
  const char* title = "WeaveBound";
  bool request_vulkan = true;
  bool visible = true;
  bool vulkan_debug_layers = false;
  /** 與 `rhi::DeviceDesc::enable_lit_demo` 對齊（陰影／HDR／bloom 示範）。 */
  bool enable_lit_demo = false;
};

class Application {
 public:
  Application() = default;
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  bool startup(const ApplicationConfig& cfg);
  void shutdown();

  /** pump_events + 計算 dt；若視窗關閉回傳 false。 */
  bool tick(float* out_dt_seconds);

  /** 固定時間步（預設 1/60 s）呼叫，用於物理等；可設空函式關閉。 */
  void set_fixed_timestep_seconds(float s) { fixed_dt_ = s > 1e-6f ? s : (1.f / 60.f); }
  void set_fixed_update(std::function<void(float fixed_dt)> fn) { fixed_update_ = std::move(fn); }

  platform::IWindow* window() const { return window_.get(); }
  rhi::IDevice* device() const { return device_.get(); }
  ecs::World& world() { return world_; }
  const ecs::World& world() const { return world_; }

 private:
  std::unique_ptr<platform::IWindow> window_;
  std::unique_ptr<rhi::IDevice> device_;
  std::unique_ptr<platform::IClock> clock_;
  double last_elapsed_{0};
  ecs::World world_;
  bool started_{false};
  float fixed_dt_{1.f / 60.f};
  float fixed_accum_{0.f};
  std::function<void(float)> fixed_update_{};
};

}  // namespace weavebound::engine
