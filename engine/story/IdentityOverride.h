#pragma once

#include <nlohmann/json.hpp>

namespace engine::story {

class IdentityOverride {
public:
  void apply(nlohmann::ordered_json& worldState, const nlohmann::ordered_json& overlay);
  void revert(nlohmann::ordered_json& worldState);

private:
  nlohmann::ordered_json backup_;
};

}  // namespace engine::story
