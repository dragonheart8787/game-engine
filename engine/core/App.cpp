#include "engine/core/App.h"

namespace engine::core {

bool App::initialize(const PlatformConfig& config) {
  if (!platform_.initialize(config)) {
    return false;
  }
  running_ = true;
  return true;
}

void App::setFixedUpdateCallback(FixedUpdateFn update) {
  fixedUpdate_ = std::move(update);
}

void App::setUpdateCallback(UpdateFn update) {
  update_ = std::move(update);
}

void App::setRenderCallback(RenderFn render) {
  render_ = std::move(render);
}

void App::setBeginFrameCallback(BeginFrameFn beginFrame) {
  beginFrame_ = std::move(beginFrame);
}

void App::setEventCallback(EventFn eventCallback) {
  eventCallback_ = std::move(eventCallback);
}

void App::setBeginRenderCallback(RenderStepFn beginRender) {
  beginRender_ = std::move(beginRender);
}

void App::setEndRenderCallback(RenderStepFn endRender) {
  endRender_ = std::move(endRender);
}

void App::run() {
  while (running_ && platform_.isRunning()) {
    platform_.pollEvents();
    if (beginFrame_) {
      beginFrame_();
    }
    if (eventCallback_) {
      eventCallback_(platform_.events());
    }

    const float delta = platform_.tick();
    accumulator_ += delta;
    while (accumulator_ >= fixedStep_) {
      if (fixedUpdate_) {
        fixedUpdate_(fixedStep_);
      }
      accumulator_ -= fixedStep_;
    }
    if (update_) {
      update_(delta);
    }
    if (beginRender_) {
      beginRender_();
    }
    if (render_) {
      render_();
    }
    if (endRender_) {
      endRender_();
    }
    platform_.present();
  }
}

void App::shutdown() {
  running_ = false;
  platform_.shutdown();
}

}  // namespace engine::core
