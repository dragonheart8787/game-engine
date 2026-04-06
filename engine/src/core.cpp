#include "game_engine/core.hpp"
#include "weavebound/version.hpp"

namespace game_engine {

int core_version_major() {
  return weavebound::version::kMajor;
}

}  // namespace game_engine
