#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "engine/ability/AbilityGraph.h"
#include "engine/math/Math.h"

namespace engine::ability {

struct AbilityEvent {
  std::string type;
  std::string abilityId;
  int emitterEntityId = -1;
  engine::math::Vec3 position{0.0f};
  nlohmann::ordered_json params;
};

class AbilityRuntime {
public:
  void loadGraph(const AbilityGraph& graph);
  void setRuntimeParams(const nlohmann::ordered_json& params);
  void trigger(const std::string& abilityId, int emitterEntityId);
  void tick(float deltaSeconds);

  const std::vector<AbilityEvent>& events() const { return events_; }
  void clearEvents() { events_.clear(); }

private:
  AbilityGraph graph_;
  std::vector<AbilityEvent> events_;
  float cooldown_ = 0.0f;
  nlohmann::ordered_json runtimeParams_ = nlohmann::ordered_json::object();
};

}  // namespace engine::ability
