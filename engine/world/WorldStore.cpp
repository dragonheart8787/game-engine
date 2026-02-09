#include "engine/world/WorldStore.h"

#include <fstream>

namespace engine::world {

WorldState WorldStore::loadFromFile(const std::string& path) {
  std::ifstream file(path);
  nlohmann::ordered_json json;
  file >> json;
  return WorldState::fromJson(json);
}

bool WorldStore::saveToFile(const WorldState& state, const std::string& path) {
  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }
  file << state.toJson().dump(2);
  return true;
}

}  // namespace engine::world
