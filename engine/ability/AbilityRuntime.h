#pragma once

#include <string>
#include <vector>

#include "engine/ability/AbilityGraph.h"

namespace engine::ability {

struct AbilityEvent {
  std::string type;
  std::string payload;
};

class AbilityRuntime {
public:
  void loadGraph(const AbilityGraph& graph);
  void trigger(const std::string& abilityId);
  void tick(float deltaSeconds);

  const std::vector<AbilityEvent>& events() const { return events_; }
  void clearEvents() { events_.clear(); }

private:
  AbilityGraph graph_;
  std::vector<AbilityEvent> events_;
};

}  // namespace engine::ability
