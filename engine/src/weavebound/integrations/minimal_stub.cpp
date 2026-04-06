#include "weavebound/integrations/minimal.hpp"

#if WEAVEBOUND_WITH_JOLT
#include "weavebound/physics/jolt_world.hpp"
#endif

namespace weavebound::integrations {

bool physics_step_minimal(float fixed_dt_seconds) {
#if WEAVEBOUND_WITH_JOLT
  static bool s_ready = false;
  if (!s_ready) {
    if (!physics::jolt::init()) {
      return false;
    }
    s_ready = true;
  }
  return physics::jolt::step(fixed_dt_seconds);
#else
  (void)fixed_dt_seconds;
  return true;
#endif
}

bool audio_tick_minimal() {
  return true;
}

bool lua_host_smoke() {
  return true;
}

}  // namespace weavebound::integrations
