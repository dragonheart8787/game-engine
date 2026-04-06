#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "weavebound/ecs/scene_types.hpp"

namespace weavebound::ecs {

struct Entity {
  std::uint32_t index{~0u};
  std::uint32_t generation{0};

  bool operator==(const Entity& o) const { return index == o.index && generation == o.generation; }
};

/**
 * 最小 SoA ECS：Transform + 可選渲染／相機／光源元件（M3 前單執行緒）。
 */
class World {
 public:
  Entity spawn();
  void destroy(Entity e);
  bool alive(Entity e) const;

  Transform3& transform(Entity e);
  const Transform3& transform(Entity e) const;

  void set_aabb(Entity e, Aabb box);
  void clear_aabb(Entity e);
  bool has_aabb(Entity e) const;
  Aabb& aabb_mut(Entity e);
  const Aabb& aabb_of(Entity e) const;

  void set_mesh_renderer(Entity e, MeshRenderer m);
  void clear_mesh_renderer(Entity e);
  bool has_mesh_renderer(Entity e) const;
  MeshRenderer& mesh_renderer_mut(Entity e);
  const MeshRenderer& mesh_renderer_of(Entity e) const;

  void set_camera(Entity e, Camera c);
  void clear_camera(Entity e);
  bool has_camera(Entity e) const;
  Camera& camera_mut(Entity e);
  const Camera& camera_of(Entity e) const;

  void set_directional_light(Entity e, DirectionalLight L);
  void clear_directional_light(Entity e);
  bool has_directional_light(Entity e) const;
  DirectionalLight& directional_light_mut(Entity e);
  const DirectionalLight& directional_light_of(Entity e) const;

  void for_each_entity_with_aabb(const std::function<void(Entity, const Aabb&)>& fn) const;

  /** 所有存活實體及其 Transform（用於場景序列化等）。 */
  void for_each_living_with_transform(const std::function<void(Entity, const Transform3&)>& fn) const;

  std::size_t living_count() const { return living_; }

 private:
  void ensure_slot(std::uint32_t idx);

  std::vector<bool> alive_;
  std::vector<std::uint32_t> generation_;
  std::vector<Transform3> transforms_;
  std::vector<std::uint32_t> free_list_;
  std::size_t living_{0};

  std::vector<std::uint8_t> has_aabb_{};
  std::vector<Aabb> aabbs_{};
  std::vector<std::uint8_t> has_mesh_{};
  std::vector<MeshRenderer> meshes_{};
  std::vector<std::uint8_t> has_camera_{};
  std::vector<Camera> cameras_{};
  std::vector<std::uint8_t> has_light_{};
  std::vector<DirectionalLight> lights_{};
};

}  // namespace weavebound::ecs
