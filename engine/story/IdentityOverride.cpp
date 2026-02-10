#include "engine/story/IdentityOverride.h"

namespace engine::story {

void IdentityOverride::apply(nlohmann::ordered_json& worldState, const nlohmann::ordered_json& overlay) {
  backup_ = worldState;
  if (overlay.contains("overrideType") && overlay["overrideType"] == "Lens") {
    return;
  }
  for (auto it = overlay.begin(); it != overlay.end(); ++it) {
    worldState[it.key()] = it.value();
  }
}

void IdentityOverride::revert(nlohmann::ordered_json& worldState) {
  if (!backup_.is_null()) {
    worldState = backup_;
  }
}

}  // namespace engine::story
