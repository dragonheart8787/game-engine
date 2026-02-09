#include <cassert>
#include <iostream>

#include "engine/world/WorldDelta.h"
#include "engine/world/WorldHasher.h"
#include "engine/world/WorldState.h"

int main() {
  nlohmann::ordered_json stateJson = {
      {"seed", 42},
      {"regions", nlohmann::ordered_json::array()},
      {"cityState", {
          {"powerLevel", 1},
          {"lockdown", false},
          {"destruction", 0},
          {"alerts", 0}
      }},
      {"characters", nlohmann::ordered_json::array()},
      {"eventFlags", nlohmann::ordered_json::object()}
  };

  engine::world::WorldState state = engine::world::WorldState::fromJson(stateJson);
  const std::uint64_t hashA = engine::world::WorldHasher::hash(state);

  nlohmann::ordered_json deltaJson = nlohmann::ordered_json::array({
      {{"op", "set"}, {"path", "/cityState/powerLevel"}, {"value", 2}}
  });
  engine::world::WorldDelta delta = engine::world::WorldDelta::fromJson(deltaJson);

  auto json = state.toJson();
  delta.apply(json);
  state.setJson(json);

  const std::uint64_t hashB = engine::world::WorldHasher::hash(state);

  assert(hashA != hashB);
  std::cout << "Worldstate tests passed" << '\n';
  return 0;
}
