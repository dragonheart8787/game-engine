#include "weavebound/scene/scene_binary.hpp"

#include <cstring>

namespace weavebound::scene {

namespace {

struct EntityRecord {
  std::uint32_t index{};
  std::uint32_t generation{};
  float pos[3]{};
};

}  // namespace

std::vector<std::uint8_t> bake_scene_binary(const ecs::World& world) {
  std::vector<EntityRecord> rec;
  world.for_each_living_with_transform([&](ecs::Entity e, const ecs::Transform3& t) {
    EntityRecord r{};
    r.index = e.index;
    r.generation = e.generation;
    r.pos[0] = t.position[0];
    r.pos[1] = t.position[1];
    r.pos[2] = t.position[2];
    rec.push_back(r);
  });

  SceneFileHeader h{};
  h.entity_count = static_cast<std::uint32_t>(rec.size());
  std::vector<std::uint8_t> out(sizeof(h) + rec.size() * sizeof(EntityRecord));
  std::memcpy(out.data(), &h, sizeof(h));
  if (!rec.empty()) {
    std::memcpy(out.data() + sizeof(h), rec.data(), rec.size() * sizeof(EntityRecord));
  }
  return out;
}

bool load_scene_binary_into(ecs::World& world, const std::uint8_t* data, std::size_t size) {
  if (data == nullptr || size < sizeof(SceneFileHeader)) {
    return false;
  }
  SceneFileHeader h{};
  std::memcpy(&h, data, sizeof(h));
  if (h.magic != kWbsceneMagic || h.version != 1) {
    return false;
  }
  const std::size_t need = sizeof(h) + static_cast<std::size_t>(h.entity_count) * sizeof(EntityRecord);
  if (size < need) {
    return false;
  }
  const auto* rec = reinterpret_cast<const EntityRecord*>(data + sizeof(h));
  for (std::uint32_t i = 0; i < h.entity_count; ++i) {
    ecs::Entity e = world.spawn();
    (void)e;
    auto& t = world.transform(e);
    t.position[0] = rec[i].pos[0];
    t.position[1] = rec[i].pos[1];
    t.position[2] = rec[i].pos[2];
  }
  return true;
}

}  // namespace weavebound::scene
