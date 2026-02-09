#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace engine::world {

struct Region {
  std::string id;
  std::string biomeType;
  std::string controlFaction;
  int dangerLevel = 0;
  std::string weatherProfile;
};

struct CityState {
  int powerLevel = 0;
  bool lockdown = false;
  int destruction = 0;
  int alerts = 0;
};

struct CharacterState {
  std::string id;
  bool alive = true;
  std::string faction;
  nlohmann::ordered_json trust;
  nlohmann::ordered_json flags;
};

class WorldState {
public:
  static WorldState fromJson(const nlohmann::ordered_json& json);
  nlohmann::ordered_json toJson() const;

  std::uint64_t seed() const { return seed_; }
  const std::vector<Region>& regions() const { return regions_; }
  const CityState& cityState() const { return cityState_; }
  const std::vector<CharacterState>& characters() const { return characters_; }
  const nlohmann::ordered_json& eventFlags() const { return eventFlags_; }

  void setJson(const nlohmann::ordered_json& json);
  const nlohmann::ordered_json& rawJson() const { return rawJson_; }

private:
  std::uint64_t seed_ = 0;
  std::vector<Region> regions_;
  CityState cityState_{};
  std::vector<CharacterState> characters_;
  nlohmann::ordered_json eventFlags_ = nlohmann::ordered_json::object();
  nlohmann::ordered_json rawJson_ = nlohmann::ordered_json::object();
};

}  // namespace engine::world
