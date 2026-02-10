#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace engine::ability {

struct AbilityNode {
  std::string id;
  std::string type;
  nlohmann::ordered_json params;
};

struct AbilityEdge {
  std::string from;
  std::string to;
};

class AbilityGraph {
public:
  static AbilityGraph fromJson(const nlohmann::ordered_json& json);

  const std::vector<AbilityNode>& nodes() const { return nodes_; }
  const std::vector<AbilityEdge>& edges() const { return edges_; }

private:
  std::vector<AbilityNode> nodes_;
  std::vector<AbilityEdge> edges_;
};

}  // namespace engine::ability
