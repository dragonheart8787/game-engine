#pragma once

#include <functional>

#include "engine/core/Platform.h"

namespace engine::core {

class App {
public:
  using UpdateFn = std::function<void(float)>;
  using FixedUpdateFn = std::function<void(float)>;
  using RenderFn = std::function<void()>;
  using BeginFrameFn = std::function<void()>;
  using EventFn = std::function<void(const std::vector<PlatformEvent>&)>;
  using RenderStepFn = std::function<void()>;

  bool initialize(const PlatformConfig& config);
  void setFixedUpdateCallback(FixedUpdateFn update);
  void setUpdateCallback(UpdateFn update);
  void setRenderCallback(RenderFn render);
  void setBeginFrameCallback(BeginFrameFn beginFrame);
  void setEventCallback(EventFn eventCallback);
  void setBeginRenderCallback(RenderStepFn beginRender);
  void setEndRenderCallback(RenderStepFn endRender);
  void run();
  void shutdown();

  void requestQuit() { platform_.requestQuit(); }
  float timeSeconds() const { return platform_.timeSeconds(); }
  Platform& platform() { return platform_; }

private:
  Platform platform_{};
  FixedUpdateFn fixedUpdate_;
  UpdateFn update_;
  RenderFn render_;
  BeginFrameFn beginFrame_;
  EventFn eventCallback_;
  RenderStepFn beginRender_;
  RenderStepFn endRender_;
  bool running_ = false;
  float fixedStep_ = 1.0f / 60.0f;
  float accumulator_ = 0.0f;
};

}  // namespace engine::core
