#include "engine/ability/AbilityRuntime.h"

namespace engine::ability {

void AbilityRuntime::loadGraph(const AbilityGraph& graph) {
  graph_ = graph;
}

void AbilityRuntime::setRuntimeParams(const nlohmann::ordered_json& params) {
  runtimeParams_ = params;
}

void AbilityRuntime::trigger(const std::string& abilityId, int emitterEntityId) {
  if (cooldown_ > 0.0f) {
    return;
  }
  events_.push_back({"AbilityTriggered", abilityId, emitterEntityId, {0.0f, 0.0f, 0.0f}, runtimeParams_});
  cooldown_ = 0.25f;
}

void AbilityRuntime::tick(float deltaSeconds) {
  if (cooldown_ > 0.0f) {
    cooldown_ -= deltaSeconds;
  }
  for (const auto& node : graph_.nodes()) {
    auto params = node.params;
    for (auto it = runtimeParams_.begin(); it != runtimeParams_.end(); ++it) {
      params[it.key()] = it.value();
    }
    if (node.type == "Spawn") {
      events_.push_back({"SpawnTrail", node.id, -1, {0.0f, 0.0f, 0.0f}, params});
    } else if (node.type == "Path") {
      events_.push_back({"SpawnBeam", node.id, -1, {0.0f, 0.0f, 0.0f}, params});
    } else if (node.type == "Affect") {
      events_.push_back({"ApplyStatus", node.id, -1, {0.0f, 0.0f, 0.0f}, params});
    } else if (node.type == "Interact") {
      events_.push_back({"ApplyImpulse", node.id, -1, {0.0f, 0.0f, 0.0f}, params});
    }
  }
}

}  // namespace engine::ability
