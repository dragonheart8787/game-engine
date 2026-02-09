#include "engine/world/WorldHasher.h"

namespace engine::world {

std::uint64_t WorldHasher::hash(const WorldState& state) {
  const std::string payload = state.toJson().dump();
  std::uint64_t hash = 1469598103934665603ull;
  for (const char c : payload) {
    hash ^= static_cast<std::uint64_t>(c);
    hash *= 1099511628211ull;
  }
  return hash;
}

}  // namespace engine::world
