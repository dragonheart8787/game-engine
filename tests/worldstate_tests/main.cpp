#include <cassert>
#include <filesystem>
#include <iostream>
#include <vector>

#include <nlohmann/json.hpp>

#include "engine/world/WorldDelta.h"
#include "engine/world/WorldHasher.h"
#include "engine/world/WorldState.h"
#include "engine/world/WorldStore.h"

int main() {
  nlohmann::ordered_json stateJson = {
      {"seed", 42},
      {"regions", nlohmann::ordered_json::array()},
      {"cityState", {
          {"powerLevel", 1},
          {"lockdown", false},
          {"destruction", 0},
          {"alerts", 0}
      }},
      {"characters", nlohmann::ordered_json::array()},
      {"eventFlags", nlohmann::ordered_json::object()}
  };

  std::string error;
  engine::world::WorldState state;
  const bool loaded = engine::world::WorldState::tryFromJson(stateJson, state, error);
  assert(loaded);
  const std::uint64_t hashA = engine::world::WorldHasher::hash(state);

  nlohmann::ordered_json deltaJson = nlohmann::ordered_json::array({
      {{"op", "set"}, {"path", "/cityState/powerLevel"}, {"value", 2}}
  });
  engine::world::WorldDelta delta = engine::world::WorldDelta::fromJson(deltaJson);

  auto json = state.toJson();
  const auto result = delta.apply(json);
  assert(result.ok);
  state.setJson(json);

  const std::uint64_t hashB = engine::world::WorldHasher::hash(state);
  assert(hashA != hashB);

  // Deterministic hash: quantized floats should produce same hash.
  nlohmann::ordered_json floatJson = state.toJson();
  floatJson["cityState"]["powerLevel"] = 1.00001;
  engine::world::WorldState stateFloat = engine::world::WorldState::fromJson(floatJson);
  const std::uint64_t hashFloat = engine::world::WorldHasher::hash(stateFloat);
  assert(hashFloat == engine::world::WorldHasher::hash(engine::world::WorldState::fromJson(floatJson)));

  // Replay determinism: apply same delta sequence yields same hash.
  std::vector<engine::world::WorldDelta> replayOps = {delta};
  auto replayJsonA = stateJson;
  for (const auto& op : replayOps) {
    const auto replayResult = op.apply(replayJsonA);
    assert(replayResult.ok);
  }
  engine::world::WorldState replayStateA = engine::world::WorldState::fromJson(replayJsonA);
  const std::uint64_t replayHashA = engine::world::WorldHasher::hash(replayStateA);

  auto replayJsonB = stateJson;
  for (const auto& op : replayOps) {
    const auto replayResult = op.apply(replayJsonB);
    assert(replayResult.ok);
  }
  engine::world::WorldState replayStateB = engine::world::WorldState::fromJson(replayJsonB);
  const std::uint64_t replayHashB = engine::world::WorldHasher::hash(replayStateB);
  assert(replayHashA == replayHashB);
  const std::uint64_t goldenHash = 14709414181201912966ull;
  assert(replayHashA == goldenHash);

  // Merge conflict behavior.
  engine::world::WorldDelta delta2 = engine::world::WorldDelta::fromJson(deltaJson);
  const bool merged = delta.merge(delta2, engine::world::WorldDelta::ConflictPolicy::LastWriteWins);
  assert(merged);
  const bool rejected = delta.merge(delta2, engine::world::WorldDelta::ConflictPolicy::RejectOnConflict);
  assert(!rejected);

  // Validation + rollback + journal.
  const std::string journalPath = "tests/worldstate_tests/world_delta_log.jsonl";
  std::filesystem::remove(journalPath);
  nlohmann::ordered_json badDeltaJson = nlohmann::ordered_json::array({
      {{"op", "inc"}, {"path", "/cityState/unknown"}, {"value", 2}}
  });
  engine::world::WorldDelta badDelta = engine::world::WorldDelta::fromJson(badDeltaJson);
  engine::world::WorldState beforeApply = engine::world::WorldState::fromJson(stateJson);
  const std::uint64_t beforeHash = engine::world::WorldHasher::hash(beforeApply);
  engine::world::WorldStore::JournalContext ctx;
  ctx.seq = 1;
  ctx.tsFixedTick = 10;
  ctx.seed = 42;
  ctx.storyId = "test_story";
  const auto applyResult = engine::world::WorldStore::applyDeltaWithJournal(beforeApply, badDelta, journalPath, ctx);
  assert(!applyResult.ok);
  assert(beforeHash == engine::world::WorldHasher::hash(beforeApply));
  assert(std::filesystem::exists(journalPath));

  std::cout << "Worldstate tests passed" << '\n';
  return 0;
}
