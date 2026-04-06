#include "weavebound/engine/application.hpp"

#include "weavebound/observability/frame_meter.hpp"

namespace weavebound::engine {

Application::~Application() { shutdown(); }

bool Application::startup(const ApplicationConfig& cfg) {
  shutdown();
  world_ = ecs::World{};

  platform::WindowDesc wd{};
  wd.width_px = cfg.width_px;
  wd.height_px = cfg.height_px;
  wd.title = cfg.title;
  wd.visible = cfg.visible;

#if defined(_WIN32)
  window_ = platform::create_win32_window(wd);
#else
  window_ = platform::create_stub_window(wd);
#endif

  clock_ = platform::create_std_clock();
  last_elapsed_ = clock_->elapsed_seconds();

  rhi::DeviceDesc dd{};
  dd.surface_target = window_.get();
  dd.backend = rhi::Backend::Vulkan;
  dd.enable_debug_layers = cfg.vulkan_debug_layers;
  dd.enable_lit_demo = cfg.enable_lit_demo;
  if (cfg.request_vulkan) {
    device_ = rhi::create_device(dd);
  } else {
    device_.reset();
  }

  started_ = window_ != nullptr;
  return started_;
}

void Application::shutdown() {
  device_.reset();
  window_.reset();
  clock_.reset();
  started_ = false;
}

bool Application::tick(float* out_dt_seconds) {
  if (!started_ || !window_) {
    return false;
  }
  window_->pump_events();
  if (!window_->is_open()) {
    return false;
  }
  const double now = clock_->elapsed_seconds();
  const float dt = static_cast<float>(now - last_elapsed_);
  last_elapsed_ = now;
  if (out_dt_seconds) {
    *out_dt_seconds = dt;
  }
  observability::record_frame_dt_seconds(dt);
  if (fixed_update_) {
    fixed_accum_ += dt;
    const float step = fixed_dt_;
    while (fixed_accum_ >= step) {
      fixed_update_(step);
      fixed_accum_ -= step;
    }
  }
  return true;
}

}  // namespace weavebound::engine
