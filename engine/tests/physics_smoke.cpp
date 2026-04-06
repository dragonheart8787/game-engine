#include "weavebound/engine/application.hpp"
#include "weavebound/integrations/minimal.hpp"

#if WEAVEBOUND_WITH_JOLT
#include "weavebound/physics/jolt_world.hpp"
#endif

#include <atomic>
#include <chrono>
#include <thread>

int main() {
  using namespace weavebound;

  engine::ApplicationConfig cfg;
  cfg.visible = false;
  cfg.request_vulkan = false;

  engine::Application app;
  if (!app.startup(cfg)) {
    return 1;
  }

  std::atomic<int> steps{0};
  app.set_fixed_timestep_seconds(1.f / 60.f);
  app.set_fixed_update([&](float step) {
    if (!integrations::physics_step_minimal(step)) {
      return;
    }
    steps.fetch_add(1, std::memory_order_relaxed);
  });

  float dt = 0.f;
  for (int i = 0; i < 8; ++i) {
    if (!app.tick(&dt)) {
      return 2;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(12));
  }

  if (steps.load(std::memory_order_relaxed) <= 0) {
    return 3;
  }

#if WEAVEBOUND_WITH_JOLT
  if (!physics::jolt::raycast_down_hit_plane()) {
    return 4;
  }
  physics::jolt::shutdown();
#endif

  return 0;
}
