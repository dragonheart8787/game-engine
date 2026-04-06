#include "weavebound/ecs/registry.hpp"

namespace weavebound::ecs {

void World::ensure_slot(std::uint32_t idx) {
  if (has_aabb_.size() <= idx) {
    const std::size_t n = static_cast<std::size_t>(idx) + 1;
    has_aabb_.resize(n);
    aabbs_.resize(n);
    has_mesh_.resize(n);
    meshes_.resize(n);
    has_camera_.resize(n);
    cameras_.resize(n);
    has_light_.resize(n);
    lights_.resize(n);
  }
}

Entity World::spawn() {
  std::uint32_t idx = 0;
  if (!free_list_.empty()) {
    idx = free_list_.back();
    free_list_.pop_back();
    alive_[idx] = true;
  } else {
    idx = static_cast<std::uint32_t>(alive_.size());
    alive_.push_back(true);
    generation_.push_back(0);
    transforms_.emplace_back();
  }
  ensure_slot(idx);
  has_aabb_[idx] = 0;
  has_mesh_[idx] = 0;
  has_camera_[idx] = 0;
  has_light_[idx] = 0;
  ++living_;
  return Entity{idx, generation_[idx]};
}

void World::destroy(Entity e) {
  if (!alive(e)) {
    return;
  }
  const std::uint32_t idx = e.index;
  alive_[idx] = false;
  ++generation_[idx];
  free_list_.push_back(idx);
  transforms_[idx] = Transform3{};
  if (idx < has_aabb_.size()) {
    has_aabb_[idx] = 0;
    has_mesh_[idx] = 0;
    has_camera_[idx] = 0;
    has_light_[idx] = 0;
  }
  if (living_ > 0) {
    --living_;
  }
}

bool World::alive(Entity e) const {
  if (e.index >= alive_.size()) {
    return false;
  }
  return alive_[e.index] && generation_[e.index] == e.generation;
}

Transform3& World::transform(Entity e) {
  if (!alive(e)) {
    static Transform3 dead{};
    return dead;
  }
  return transforms_[e.index];
}

const Transform3& World::transform(Entity e) const {
  if (!alive(e)) {
    static const Transform3 dead{};
    return dead;
  }
  return transforms_[e.index];
}

void World::set_aabb(Entity e, Aabb box) {
  if (!alive(e)) {
    return;
  }
  ensure_slot(e.index);
  aabbs_[e.index] = box;
  has_aabb_[e.index] = 1;
}

void World::clear_aabb(Entity e) {
  if (e.index >= has_aabb_.size()) {
    return;
  }
  has_aabb_[e.index] = 0;
}

bool World::has_aabb(Entity e) const {
  return alive(e) && e.index < has_aabb_.size() && has_aabb_[e.index] != 0;
}

Aabb& World::aabb_mut(Entity e) {
  static Aabb dead{};
  if (!alive(e)) {
    return dead;
  }
  ensure_slot(e.index);
  has_aabb_[e.index] = 1;
  return aabbs_[e.index];
}

const Aabb& World::aabb_of(Entity e) const {
  static const Aabb dead{};
  if (!has_aabb(e)) {
    return dead;
  }
  return aabbs_[e.index];
}

void World::set_mesh_renderer(Entity e, MeshRenderer m) {
  if (!alive(e)) {
    return;
  }
  ensure_slot(e.index);
  meshes_[e.index] = m;
  has_mesh_[e.index] = 1;
}

void World::clear_mesh_renderer(Entity e) {
  if (e.index >= has_mesh_.size()) {
    return;
  }
  has_mesh_[e.index] = 0;
}

bool World::has_mesh_renderer(Entity e) const {
  return alive(e) && e.index < has_mesh_.size() && has_mesh_[e.index] != 0;
}

MeshRenderer& World::mesh_renderer_mut(Entity e) {
  static MeshRenderer dead{};
  if (!alive(e)) {
    return dead;
  }
  ensure_slot(e.index);
  has_mesh_[e.index] = 1;
  return meshes_[e.index];
}

const MeshRenderer& World::mesh_renderer_of(Entity e) const {
  static const MeshRenderer dead{};
  if (!has_mesh_renderer(e)) {
    return dead;
  }
  return meshes_[e.index];
}

void World::set_camera(Entity e, Camera c) {
  if (!alive(e)) {
    return;
  }
  ensure_slot(e.index);
  cameras_[e.index] = c;
  has_camera_[e.index] = 1;
}

void World::clear_camera(Entity e) {
  if (e.index >= has_camera_.size()) {
    return;
  }
  has_camera_[e.index] = 0;
}

bool World::has_camera(Entity e) const {
  return alive(e) && e.index < has_camera_.size() && has_camera_[e.index] != 0;
}

Camera& World::camera_mut(Entity e) {
  static Camera dead{};
  if (!alive(e)) {
    return dead;
  }
  ensure_slot(e.index);
  has_camera_[e.index] = 1;
  return cameras_[e.index];
}

const Camera& World::camera_of(Entity e) const {
  static const Camera dead{};
  if (!has_camera(e)) {
    return dead;
  }
  return cameras_[e.index];
}

void World::set_directional_light(Entity e, DirectionalLight L) {
  if (!alive(e)) {
    return;
  }
  ensure_slot(e.index);
  lights_[e.index] = L;
  has_light_[e.index] = 1;
}

void World::clear_directional_light(Entity e) {
  if (e.index >= has_light_.size()) {
    return;
  }
  has_light_[e.index] = 0;
}

bool World::has_directional_light(Entity e) const {
  return alive(e) && e.index < has_light_.size() && has_light_[e.index] != 0;
}

DirectionalLight& World::directional_light_mut(Entity e) {
  static DirectionalLight dead{};
  if (!alive(e)) {
    return dead;
  }
  ensure_slot(e.index);
  has_light_[e.index] = 1;
  return lights_[e.index];
}

const DirectionalLight& World::directional_light_of(Entity e) const {
  static const DirectionalLight dead{};
  if (!has_directional_light(e)) {
    return dead;
  }
  return lights_[e.index];
}

void World::for_each_entity_with_aabb(const std::function<void(Entity, const Aabb&)>& fn) const {
  for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(alive_.size()); ++i) {
    if (!alive_[i] || i >= has_aabb_.size() || !has_aabb_[i]) {
      continue;
    }
    fn(Entity{i, generation_[i]}, aabbs_[i]);
  }
}

void World::for_each_living_with_transform(const std::function<void(Entity, const Transform3&)>& fn) const {
  for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(alive_.size()); ++i) {
    if (!alive_[i]) {
      continue;
    }
    fn(Entity{i, generation_[i]}, transforms_[i]);
  }
}

}  // namespace weavebound::ecs
