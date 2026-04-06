#include "weavebound/audio/mixer.hpp"
#include "weavebound/integrations/minimal.hpp"
#include "weavebound/scripting/lua_host.hpp"

int main() {
  using namespace weavebound;

  auto mix = audio::create_miniaudio_mixer();
  if (mix) {
    mix->pump();
  }

  auto lua = scripting::create_lua_host_stub();
  if (!lua || !lua->init()) {
    return 1;
  }
  lua->shutdown();

  return integrations::physics_step_minimal() ? 0 : 2;
}
