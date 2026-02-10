#include "engine/world/WorldState.h"

namespace engine::world {

WorldState WorldState::fromJson(const nlohmann::ordered_json& json) {
  WorldState state;
  state.setJson(json);
  return state;
}

bool WorldState::tryFromJson(
    const nlohmann::ordered_json& json,
    WorldState& out,
    std::string& error) {
  if (!json.contains("seed")) {
    error = "/seed missing";
    return false;
  }
  if (!json.contains("regions")) {
    error = "/regions missing";
    return false;
  }
  out.setJson(json);
  return true;
}

void WorldState::setJson(const nlohmann::ordered_json& json) {
  rawJson_ = json;
  seed_ = rawJson_.value("seed", 0u);

  regions_.clear();
  for (const auto& regionJson : rawJson_.value("regions", nlohmann::ordered_json::array())) {
    Region region;
    region.id = regionJson.value("id", "");
    region.biomeType = regionJson.value("biomeType", "");
    region.controlFaction = regionJson.value("controlFaction", "");
    region.dangerLevel = regionJson.value("dangerLevel", 0);
    region.weatherProfile = regionJson.value("weatherProfile", "");
    regions_.push_back(region);
  }

  const auto cityJson = rawJson_.value("cityState", nlohmann::ordered_json::object());
  cityState_.powerLevel = cityJson.value("powerLevel", 0);
  cityState_.lockdown = cityJson.value("lockdown", false);
  cityState_.destruction = cityJson.value("destruction", 0);
  cityState_.alerts = cityJson.value("alerts", 0);

  characters_.clear();
  for (const auto& characterJson : rawJson_.value("characters", nlohmann::ordered_json::array())) {
    CharacterState character;
    character.id = characterJson.value("id", "");
    character.alive = characterJson.value("alive", true);
    character.faction = characterJson.value("faction", "");
    character.trust = characterJson.value("trust", nlohmann::ordered_json::object());
    character.flags = characterJson.value("flags", nlohmann::ordered_json::object());
    characters_.push_back(character);
  }

  eventFlags_ = rawJson_.value("eventFlags", nlohmann::ordered_json::object());
}

nlohmann::ordered_json WorldState::toJson() const {
  return rawJson_;
}

const Region* WorldState::findRegion(const std::string& id) const {
  for (const auto& region : regions_) {
    if (region.id == id) {
      return &region;
    }
  }
  return nullptr;
}

const CharacterState* WorldState::getCharacter(const std::string& id) const {
  for (const auto& character : characters_) {
    if (character.id == id) {
      return &character;
    }
  }
  return nullptr;
}

}  // namespace engine::world
