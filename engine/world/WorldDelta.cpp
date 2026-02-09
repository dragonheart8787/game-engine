#include "engine/world/WorldDelta.h"

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

void WorldDelta::apply(nlohmann::ordered_json& target) const {
  for (const auto& op : ops_) {
    const nlohmann::json::json_pointer pointer(op.path);
    if (op.op == "set") {
      target[pointer] = op.value;
    } else if (op.op == "add") {
      target[pointer].push_back(op.value);
    } else if (op.op == "inc") {
      target[pointer] = target.value(pointer, 0) + op.value.get<int>();
    } else if (op.op == "remove") {
      target.erase(pointer);
    }
  }
}

}  // namespace engine::world
