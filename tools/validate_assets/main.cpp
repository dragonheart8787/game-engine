#include <filesystem>
#include <iostream>
#include <set>

#include "engine/tools/AssetIO.h"

namespace fs = std::filesystem;

int main() {
  bool ok = true;

  const fs::path abilitiesDir = "assets/abilities";
  for (const auto& entry : fs::directory_iterator(abilitiesDir)) {
    if (entry.path().extension() != ".json") {
      continue;
    }
    const auto json = engine::tools::loadJson(entry.path().string());
    std::set<std::string> nodeIds;
    for (const auto& node : json.value("nodes", nlohmann::ordered_json::array())) {
      const std::string type = node.value("type", "");
      static const std::set<std::string> allowed = {
          "Shape", "Path", "Constraint", "Spawn", "Affect", "Cost", "Cooldown", "Interact"};
      if (!allowed.count(type)) {
        std::cerr << "Invalid ability node type in " << entry.path() << ": " << type << "\n";
        ok = false;
      }
      nodeIds.insert(node.value("id", ""));
    }
    for (const auto& edge : json.value("edges", nlohmann::ordered_json::array())) {
      if (!nodeIds.count(edge.value("from", "")) || !nodeIds.count(edge.value("to", ""))) {
        std::cerr << "Invalid edge in " << entry.path() << "\n";
        ok = false;
      }
    }
  }

  const fs::path storyDir = "assets/story";
  for (const auto& entry : fs::directory_iterator(storyDir)) {
    if (entry.path().extension() != ".json") {
      continue;
    }
    const auto json = engine::tools::loadJson(entry.path().string());
    if (!json.contains("beats") || json["beats"].empty()) {
      std::cerr << "Story has no beats: " << entry.path() << "\n";
      ok = false;
    }
  }

  if (!ok) {
    return 1;
  }
  std::cout << "Asset validation passed\n";
  return 0;
}
