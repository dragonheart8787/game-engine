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

enum class ControlMode {
  Hold,
  Guide,
  Punch
};

struct ControlMask {
  bool allowMove = true;
  bool allowLook = true;
  bool allowAbility = true;
  bool allowUi = true;
};

class Director {
public:
  void loadStory(const StoryAsset& asset);
  bool checkEntryConditions(const nlohmann::ordered_json& worldState) const;
  engine::world::WorldDelta runBeats();
  void tick(float deltaSeconds);
  ControlMask controlMask() const { return mask_; }

private:
  StoryAsset asset_;
  ControlMask mask_{};
  float holdTimer_ = 0.0f;
};

StoryAsset loadStoryAsset(const nlohmann::ordered_json& json);

}  // namespace engine::story
