#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "engine/world/WorldDelta.h"

namespace engine::story {

struct StoryBeat {
  std::string cameraPlan;
  std::string controlMode;
  std::string objective;
  nlohmann::ordered_json triggers;
  std::string failSoft;
};

struct StoryAsset {
  std::string storyId;
  std::string mode;
  std::string overrideType;
  nlohmann::ordered_json entryConditions;
  std::vector<StoryBeat> beats;
  nlohmann::ordered_json constraints;
  nlohmann::ordered_json worldDeltaOut;
};

class Director {
public:
  void loadStory(const StoryAsset& asset);
  bool checkEntryConditions(const nlohmann::ordered_json& worldState) const;
  engine::world::WorldDelta run();

private:
  StoryAsset asset_;
};

StoryAsset loadStoryAsset(const nlohmann::ordered_json& json);

}  // namespace engine::story
