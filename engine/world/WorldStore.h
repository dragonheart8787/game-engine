#pragma once

#include <string>

#include "engine/world/WorldDelta.h"
#include "engine/world/WorldState.h"

namespace engine::world {

class WorldStore {
public:
  struct ApplyDeltaResult {
    bool ok = true;
    std::string error;
  };

  static WorldState loadFromFile(const std::string& path);
  static bool saveToFile(const WorldState& state, const std::string& path);
  static ApplyDeltaResult applyDeltaWithJournal(
      WorldState& state,
      const WorldDelta& delta,
      const std::string& journalPath);
};

}  // namespace engine::world
