#include "engine/ability/AbilityGraph.h"

namespace engine::ability {

AbilityGraph AbilityGraph::fromJson(const nlohmann::ordered_json& json) {
  AbilityGraph graph;
  for (const auto& nodeJson : json.value("nodes", nlohmann::ordered_json::array())) {
    AbilityNode node;
    node.id = nodeJson.value("id", "");
    node.type = nodeJson.value("type", "");
    node.params = nodeJson.value("params", nlohmann::ordered_json::object());
    graph.nodes_.push_back(node);
  }
  for (const auto& edgeJson : json.value("edges", nlohmann::ordered_json::array())) {
    AbilityEdge edge;
    edge.from = edgeJson.value("from", "");
    edge.to = edgeJson.value("to", "");
    graph.edges_.push_back(edge);
  }
  return graph;
}

}  // namespace engine::ability
