#include "engine/procgen/WorldGenerator.h"

namespace engine::procgen {

engine::world::WorldState WorldGenerator::generate(std::uint64_t seed) const {
  nlohmann::ordered_json json;
  json["seed"] = seed;
  json["regions"] = nlohmann::ordered_json::array({
      {
          {"id", "core"},
          {"biomeType", "city"},
          {"controlFaction", "neutral"},
          {"dangerLevel", 1},
          {"weatherProfile", "clear"}
      }
  });
  json["cityState"] = {
      {"powerLevel", 3},
      {"lockdown", false},
      {"destruction", 0},
      {"alerts", 0}
  };
  json["characters"] = nlohmann::ordered_json::array();
  json["eventFlags"] = nlohmann::ordered_json::object();
  return engine::world::WorldState::fromJson(json);
}

}  // namespace engine::procgen
