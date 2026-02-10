#include "apps/demo_thirdperson/DemoGame.h"

int main(int /*argc*/, char** /*argv*/) {
  engine::core::App app;
  engine::core::PlatformConfig config;
  config.title = "Demo Third Person";

  if (!app.initialize(config)) {
    return 1;
  }

  apps::demo_thirdperson::DemoGame game;
  if (!game.initialize(app)) {
    return 1;
  }

  app.setBeginFrameCallback([&]() { game.beginFrame(); });
  app.setEventCallback([&](const std::vector<engine::core::PlatformEvent>& events) {
    game.handleEvents(events);
  });
  app.setBeginRenderCallback([&]() { game.beginRender(); });
  app.setFixedUpdateCallback([&](float delta) { game.fixedUpdate(delta); });
  app.setUpdateCallback([&](float delta) { game.update(delta); });
  app.setRenderCallback([&]() { game.render(); });
  app.setEndRenderCallback([&]() { game.endRender(); });
  app.run();
  app.shutdown();
  return 0;
}
