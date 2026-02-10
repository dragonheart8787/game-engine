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

  struct ApplyResult {
    bool ok = true;
    std::string error;
  };

  enum class ConflictPolicy {
    LastWriteWins,
    RejectOnConflict
  };

  ApplyResult apply(nlohmann::ordered_json& target) const;
  bool merge(const WorldDelta& other, ConflictPolicy policy);
  const std::vector<DeltaOp>& operations() const { return ops_; }

private:
  std::vector<DeltaOp> ops_;
};

}  // namespace engine::world
