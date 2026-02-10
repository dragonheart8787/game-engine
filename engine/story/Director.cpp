#include "engine/story/Director.h"

namespace engine::story {

StoryAsset loadStoryAsset(const nlohmann::ordered_json& json) {
  StoryAsset asset;
  asset.storyId = json.value("storyId", "");
  asset.mode = json.value("mode", "");
  asset.overrideType = json.value("overrideType", "");
  asset.entryConditions = json.value("entryConditions", nlohmann::ordered_json::object());
  asset.constraints = json.value("constraints", nlohmann::ordered_json::object());
  asset.worldDeltaOut = json.value("worldDeltaOut", nlohmann::ordered_json::array());

  for (const auto& beatJson : json.value("beats", nlohmann::ordered_json::array())) {
    StoryBeat beat;
    beat.cameraPlan = beatJson.value("cameraPlan", "");
    beat.controlMode = beatJson.value("controlMode", "");
    beat.objective = beatJson.value("objective", "");
    beat.triggers = beatJson.value("triggers", nlohmann::ordered_json::object());
    beat.failSoft = beatJson.value("failSoft", "");
    asset.beats.push_back(beat);
  }

  return asset;
}

void Director::loadStory(const StoryAsset& asset) {
  asset_ = asset;
}

bool Director::checkEntryConditions(const nlohmann::ordered_json& worldState) const {
  for (auto it = asset_.entryConditions.begin(); it != asset_.entryConditions.end(); ++it) {
    const std::string key = it.key();
    std::string pointer = "/" + key;
    for (auto& ch : pointer) {
      if (ch == '.') {
        ch = '/';
      }
    }
    const nlohmann::json::json_pointer ptr(pointer);
    if (!worldState.contains(ptr) || worldState[ptr] != it.value()) {
      return false;
    }
  }
  return true;
}

engine::world::WorldDelta Director::runBeats() {
  if (!asset_.beats.empty()) {
    const auto& beat = asset_.beats.front();
    if (beat.controlMode == "Hold") {
      mask_.allowMove = false;
      mask_.allowLook = false;
      mask_.allowAbility = false;
      mask_.allowUi = false;
      holdTimer_ = 2.0f;
    } else if (beat.controlMode == "Guide") {
      mask_.allowMove = true;
      mask_.allowLook = false;
      mask_.allowAbility = true;
      mask_.allowUi = true;
    } else {
      mask_.allowMove = true;
      mask_.allowLook = true;
      mask_.allowAbility = true;
      mask_.allowUi = true;
    }
  }
  return engine::world::WorldDelta::fromJson(asset_.worldDeltaOut);
}

void Director::tick(float deltaSeconds) {
  if (holdTimer_ > 0.0f) {
    holdTimer_ -= deltaSeconds;
    if (holdTimer_ <= 0.0f) {
      mask_.allowMove = true;
      mask_.allowLook = true;
    }
  }
}

}  // namespace engine::story
