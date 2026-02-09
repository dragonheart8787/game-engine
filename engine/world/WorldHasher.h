#pragma once

#include <cstdint>

#include "engine/world/WorldState.h"

namespace engine::world {

class WorldHasher {
public:
  static std::uint64_t hash(const WorldState& state);
};

}  // namespace engine::world
