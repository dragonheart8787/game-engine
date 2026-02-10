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

  struct JournalContext {
    std::uint64_t seq = 0;
    std::uint64_t tsFixedTick = 0;
    std::uint64_t seed = 0;
    std::string storyId;
    std::size_t maxEntries = 1024;
  };

  static WorldState loadFromFile(const std::string& path);
  static bool saveToFile(const WorldState& state, const std::string& path);
  static ApplyDeltaResult applyDeltaWithJournal(
      WorldState& state,
      const WorldDelta& delta,
      const std::string& journalPath,
      const JournalContext& context);
};

}  // namespace engine::world
