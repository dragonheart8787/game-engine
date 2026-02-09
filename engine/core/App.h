#pragma once

#include <functional>

#include "engine/core/Platform.h"

namespace engine::core {

class App {
public:
  using UpdateFn = std::function<void(float)>;
  using RenderFn = std::function<void()>;

  bool initialize(const PlatformConfig& config);
  void setUpdateCallback(UpdateFn update);
  void setRenderCallback(RenderFn render);
  void run();
  void shutdown();

  Platform& platform() { return platform_; }

private:
  Platform platform_{};
  UpdateFn update_;
  RenderFn render_;
  bool running_ = false;
};

}  // namespace engine::core
