#include "engine/world/WorldStore.h"

#include <fstream>
#include <vector>

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

std::uint64_t hashDelta(const WorldDelta& delta) {
  const std::string payload = delta.toJson().dump();
  std::uint64_t hash = 1469598103934665603ull;
  for (const char c : payload) {
    hash ^= static_cast<std::uint64_t>(c);
    hash *= 1099511628211ull;
  }
  return hash;
}

void rotateJournalIfNeeded(const std::string& journalPath, std::size_t maxEntries) {
  std::ifstream input(journalPath);
  if (!input.is_open()) {
    return;
  }
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(input, line)) {
    lines.push_back(line);
  }
  if (lines.size() <= maxEntries) {
    return;
  }
  const std::size_t start = lines.size() - maxEntries;
  std::ofstream output(journalPath, std::ios::trunc);
  for (std::size_t i = start; i < lines.size(); ++i) {
    output << lines[i] << "\n";
  }
}

void appendJournal(
    const std::string& journalPath,
    const WorldDelta& delta,
    const std::string& result,
    const std::string& message,
    const WorldStore::JournalContext& context) {
  rotateJournalIfNeeded(journalPath, context.maxEntries);

  std::ofstream file(journalPath, std::ios::app);
  if (!file.is_open()) {
    return;
  }
  nlohmann::ordered_json entry = {
      {"version", "journal_v1"},
      {"seq", context.seq},
      {"ts_fixedTick", context.tsFixedTick},
      {"seed", context.seed},
      {"storyId", context.storyId},
      {"deltaHash", hashDelta(delta)},
      {"deltaOps", delta.toJson()},
      {"result", result},
      {"message", message}
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
    const std::string& journalPath,
    const JournalContext& context) {
  auto staged = state.toJson();
  std::string validationError;
  if (!validateDeltaAgainstState(staged, delta, validationError)) {
    appendJournal(journalPath, delta, "error", validationError, context);
    return {false, validationError};
  }

  const auto result = delta.apply(staged);
  if (!result.ok) {
    appendJournal(journalPath, delta, "error", result.error, context);
    return {false, result.error};
  }

  state.setJson(staged);
  appendJournal(journalPath, delta, "committed", "ok", context);
  return {true, ""};
}

}  // namespace engine::world
