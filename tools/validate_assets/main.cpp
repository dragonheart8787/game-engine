#include <filesystem>
#include <iostream>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "engine/tools/AssetIO.h"

namespace fs = std::filesystem;

namespace {
bool hasCycleDfs(
    const std::string& node,
    const std::unordered_map<std::string, std::vector<std::string>>& graph,
    std::unordered_set<std::string>& visiting,
    std::unordered_set<std::string>& visited) {
  if (visiting.count(node)) {
    return true;
  }
  if (visited.count(node)) {
    return false;
  }
  visiting.insert(node);
  auto it = graph.find(node);
  if (it != graph.end()) {
    for (const auto& to : it->second) {
      if (hasCycleDfs(to, graph, visiting, visited)) {
        return true;
      }
    }
  }
  visiting.erase(node);
  visited.insert(node);
  return false;
}
}  // namespace

int main() {
  bool ok = true;

  const fs::path abilitiesDir = "assets/abilities";
  for (const auto& entry : fs::directory_iterator(abilitiesDir)) {
    if (entry.path().extension() != ".json") {
      continue;
    }
    const auto json = engine::tools::loadJson(entry.path().string());
    std::set<std::string> nodeIds;
    std::unordered_map<std::string, std::vector<std::string>> graph;
    for (const auto& node : json.value("nodes", nlohmann::ordered_json::array())) {
      const std::string nodeId = node.value("id", "");
      const std::string type = node.value("type", "");
      static const std::set<std::string> allowed = {
          "Shape", "Path", "Constraint", "Spawn", "Affect", "Cost", "Cooldown", "Interact"};
      if (!allowed.count(type)) {
        std::cerr << entry.path() << " /nodes: invalid type " << type << "\n";
        ok = false;
      }
      if (!node.contains("params")) {
        std::cerr << entry.path() << " /nodes/params missing for " << nodeId << "\n";
        ok = false;
      }
      nodeIds.insert(nodeId);
    }
    for (const auto& edge : json.value("edges", nlohmann::ordered_json::array())) {
      const std::string from = edge.value("from", "");
      const std::string to = edge.value("to", "");
      if (!nodeIds.count(from) || !nodeIds.count(to)) {
        std::cerr << entry.path() << " /edges invalid reference from=" << from << " to=" << to << "\n";
        ok = false;
      }
      graph[from].push_back(to);
    }
    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;
    for (const auto& id : nodeIds) {
      if (hasCycleDfs(id, graph, visiting, visited)) {
        std::cerr << entry.path() << " /edges cycle detected\n";
        ok = false;
        break;
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
      std::cerr << entry.path() << " /beats missing\n";
      ok = false;
      continue;
    }
    for (std::size_t i = 0; i < json["beats"].size(); ++i) {
      const auto& beat = json["beats"][i];
      if (!beat.contains("triggers")) {
        std::cerr << entry.path() << " /beats/" << i << "/triggers missing\n";
        ok = false;
      }
      if (!beat.contains("controlMode")) {
        std::cerr << entry.path() << " /beats/" << i << "/controlMode missing\n";
        ok = false;
      }
    }
  }

  const auto delta = engine::tools::loadJson("assets/scenes/worlddelta.json");
  for (std::size_t i = 0; i < delta.size(); ++i) {
    const auto& op = delta[i];
    if (!op.contains("path") || !op["path"].is_string()) {
      std::cerr << "assets/scenes/worlddelta.json /" << i << "/path invalid\n";
      ok = false;
    }
    if (op.value("op", "") == "inc" && !op.contains("value")) {
      std::cerr << "assets/scenes/worlddelta.json /" << i << " inc missing value\n";
      ok = false;
    }
  }

  if (!ok) {
    return 1;
  }
  std::cout << "Asset validation passed\n";
  return 0;
}
