#include "engine/ability/AbilityRuntime.h"

namespace engine::ability {

void AbilityRuntime::loadGraph(const AbilityGraph& graph) {
  graph_ = graph;
}

void AbilityRuntime::trigger(const std::string& abilityId) {
  events_.push_back({"AbilityTriggered", abilityId});
}

void AbilityRuntime::tick(float /*deltaSeconds*/) {
  for (const auto& node : graph_.nodes()) {
    if (node.type == "Spawn") {
      events_.push_back({"Spawn", node.id});
    }
  }
}

}  // namespace engine::ability
