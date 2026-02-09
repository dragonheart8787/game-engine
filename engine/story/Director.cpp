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

bool Director::checkEntryConditions(const nlohmann::ordered_json& /*worldState*/) const {
  return true;
}

engine::world::WorldDelta Director::run() {
  return engine::world::WorldDelta::fromJson(asset_.worldDeltaOut);
}

}  // namespace engine::story
