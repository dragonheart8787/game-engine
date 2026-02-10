#include "engine/world/WorldStore.h"

#include <fstream>

namespace engine::world {

namespace {
bool validateDeltaAgainstState(const nlohmann::ordered_json& stateJson, const WorldDelta& delta, std::string& error) {
  for (const auto& op : delta.operations()) {
    try {
      const nlohmann::json::json_pointer pointer(op.path);
      if (op.op == "inc" && !stateJson.contains(pointer)) {
        error = op.path + ": missing path for inc";
        return false;
      }
      if (op.op == "inc" && !stateJson[pointer].is_number()) {
        error = op.path + ": inc target is not number";
        return false;
      }
    } catch (const std::exception& e) {
      error = op.path + ": " + e.what();
      return false;
    }
  }
  return true;
}

void appendJournal(const std::string& journalPath, const WorldDelta& delta, const std::string& status, const std::string& message) {
  std::ofstream file(journalPath, std::ios::app);
  if (!file.is_open()) {
    return;
  }
  nlohmann::ordered_json entry = {
      {"status", status},
      {"message", message},
      {"delta", delta.toJson()}
  };
  file << entry.dump() << "\n";
}
}  // namespace

WorldState WorldStore::loadFromFile(const std::string& path) {
  std::ifstream file(path);
  nlohmann::ordered_json json;
  file >> json;
  return WorldState::fromJson(json);
}

bool WorldStore::saveToFile(const WorldState& state, const std::string& path) {
  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }
  file << state.toJson().dump(2);
  return true;
}

WorldStore::ApplyDeltaResult WorldStore::applyDeltaWithJournal(
    WorldState& state,
    const WorldDelta& delta,
    const std::string& journalPath) {
  auto staged = state.toJson();
  std::string validationError;
  if (!validateDeltaAgainstState(staged, delta, validationError)) {
    appendJournal(journalPath, delta, "rejected", validationError);
    return {false, validationError};
  }

  const auto result = delta.apply(staged);
  if (!result.ok) {
    appendJournal(journalPath, delta, "failed", result.error);
    return {false, result.error};
  }

  state.setJson(staged);
  appendJournal(journalPath, delta, "committed", "ok");
  return {true, ""};
}

}  // namespace engine::world
