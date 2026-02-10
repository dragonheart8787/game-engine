#include "engine/world/WorldHasher.h"

#include <cmath>

#include <nlohmann/json.hpp>

namespace engine::world {

namespace {
void quantizeJson(nlohmann::ordered_json& json) {
  if (json.is_number_float()) {
    const double value = json.get<double>();
    const double quantized = std::round(value * 10000.0) / 10000.0;
    json = quantized;
    return;
  }
  if (json.is_array()) {
    for (auto& item : json) {
      quantizeJson(item);
    }
    return;
  }
  if (json.is_object()) {
    for (auto it = json.begin(); it != json.end(); ++it) {
      quantizeJson(it.value());
    }
  }
}
}  // namespace

std::uint64_t WorldHasher::hash(const WorldState& state) {
  nlohmann::ordered_json json = state.toJson();
  quantizeJson(json);
  const std::string payload = json.dump();
  std::uint64_t hash = 1469598103934665603ull;
  for (const char c : payload) {
    hash ^= static_cast<std::uint64_t>(c);
    hash *= 1099511628211ull;
  }
  return hash;
}

}  // namespace engine::world
