#pragma once

#include <string>

#include "engine/world/WorldState.h"

namespace engine::world {

class WorldStore {
public:
  static WorldState loadFromFile(const std::string& path);
  static bool saveToFile(const WorldState& state, const std::string& path);
};

}  // namespace engine::world
