#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace engine::world {

struct DeltaOp {
  std::string op;
  std::string path;
  nlohmann::ordered_json value;
};

class WorldDelta {
public:
  static WorldDelta fromJson(const nlohmann::ordered_json& json);
  nlohmann::ordered_json toJson() const;

  void apply(nlohmann::ordered_json& target) const;
  const std::vector<DeltaOp>& operations() const { return ops_; }

private:
  std::vector<DeltaOp> ops_;
};

}  // namespace engine::world
