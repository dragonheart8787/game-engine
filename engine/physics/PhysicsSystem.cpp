#include "engine/physics/PhysicsSystem.h"

namespace engine::physics {

RaycastHit PhysicsSystem::raycast(const engine::math::Vec3& origin, const engine::math::Vec3& /*direction*/) const {
  return RaycastHit{origin, false};
}

}  // namespace engine::physics
