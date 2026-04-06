#include "weavebound/asset/wbpak.hpp"
#include "weavebound/scene/scene_binary.hpp"

#include "weavebound/ecs/registry.hpp"

int main() {
  weavebound::ecs::World w;
  weavebound::ecs::Entity e = w.spawn();
  w.transform(e).position[0] = 1.f;
  w.transform(e).position[1] = 2.f;
  w.transform(e).position[2] = 3.f;

  const std::vector<std::uint8_t> scene = weavebound::scene::bake_scene_binary(w);
  if (scene.size() < sizeof(weavebound::scene::SceneFileHeader)) {
    return 1;
  }

  weavebound::ecs::World w2;
  if (!weavebound::scene::load_scene_binary_into(w2, scene.data(), scene.size())) {
    return 2;
  }
  if (w2.living_count() < 1) {
    return 3;
  }

  weavebound::asset::WbpakFileEntry ent{};
  ent.logical_name = "scene.wbscene";
  ent.bytes = scene;
  const std::vector<std::uint8_t> pak = weavebound::asset::build_wbpak({ent});
  if (pak.empty()) {
    return 4;
  }
  return 0;
}
