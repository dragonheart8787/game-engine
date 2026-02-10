#pragma once

#include <cstdint>

#include "engine/world/WorldState.h"

namespace engine::procgen {

class WorldGenerator {
public:
  engine::world::WorldState generate(std::uint64_t seed) const;
};

}  // namespace engine::procgen
