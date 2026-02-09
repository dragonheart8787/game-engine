#include "engine/core/App.h"

namespace engine::core {

bool App::initialize(const PlatformConfig& config) {
  if (!platform_.initialize(config)) {
    return false;
  }
  running_ = true;
  return true;
}

void App::setUpdateCallback(UpdateFn update) {
  update_ = std::move(update);
}

void App::setRenderCallback(RenderFn render) {
  render_ = std::move(render);
}

void App::run() {
  while (running_ && platform_.isRunning()) {
    const float delta = platform_.tick();
    if (update_) {
      update_(delta);
    }
    if (render_) {
      render_();
    }
    platform_.present();
  }
}

void App::shutdown() {
  running_ = false;
  platform_.shutdown();
}

}  // namespace engine::core
