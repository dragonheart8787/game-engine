#include "engine/world/WorldDelta.h"

#include <algorithm>

namespace engine::world {

WorldDelta WorldDelta::fromJson(const nlohmann::ordered_json& json) {
  WorldDelta delta;
  for (const auto& item : json) {
    DeltaOp op;
    op.op = item.value("op", "");
    op.path = item.value("path", "");
    op.value = item.value("value", nlohmann::ordered_json::object());
    delta.ops_.push_back(op);
  }
  return delta;
}

nlohmann::ordered_json WorldDelta::toJson() const {
  nlohmann::ordered_json out = nlohmann::ordered_json::array();
  for (const auto& op : ops_) {
    out.push_back({{"op", op.op}, {"path", op.path}, {"value", op.value}});
  }
  return out;
}

WorldDelta::ApplyResult WorldDelta::apply(nlohmann::ordered_json& target) const {
  nlohmann::ordered_json working = target;
  for (const auto& op : ops_) {
    try {
      const nlohmann::json::json_pointer pointer(op.path);
      if (op.op == "set") {
        working[pointer] = op.value;
      } else if (op.op == "add") {
        working[pointer].push_back(op.value);
      } else if (op.op == "inc") {
        working[pointer] = working.value(pointer, 0) + op.value.get<int>();
      } else if (op.op == "remove") {
        working.erase(pointer);
      } else {
        return {false, "Unknown op: " + op.op + " at " + op.path};
      }
    } catch (const std::exception& e) {
      return {false, op.path + ": " + e.what()};
    }
  }
  target = working;
  return {true, ""};
}

bool WorldDelta::merge(const WorldDelta& other, ConflictPolicy policy) {
  for (const auto& op : other.ops_) {
    auto it = std::find_if(
        ops_.begin(),
        ops_.end(),
        [&](const DeltaOp& existing) { return existing.path == op.path; });
    if (it != ops_.end()) {
      if (policy == ConflictPolicy::RejectOnConflict) {
        return false;
      }
      *it = op;
    } else {
      ops_.push_back(op);
    }
  }
  return true;
}

}  // namespace engine::world
