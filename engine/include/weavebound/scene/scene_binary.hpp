#pragma once

#include <cstdint>
#include <vector>

#include "weavebound/ecs/registry.hpp"

namespace weavebound::scene {

constexpr std::uint32_t kWbsceneMagic = 0x57425343u;  // 'WBSC'

struct SceneFileHeader {
  std::uint32_t magic{kWbsceneMagic};
  std::uint32_t version{1};
  std::uint32_t entity_count{0};
  std::uint32_t reserved{0};
};

/** 將 World 內含 transform 的實體序列化（簡化：僅 position）。 */
std::vector<std::uint8_t> bake_scene_binary(const ecs::World& world);

/** 載入並套用至 World（清空既有 optional 元件策略：僅 spawn 新實體）。 */
bool load_scene_binary_into(ecs::World& world, const std::uint8_t* data, std::size_t size);

}  // namespace weavebound::scene
