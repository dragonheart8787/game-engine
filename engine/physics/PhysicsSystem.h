#pragma once

#include "engine/math/Math.h"

namespace engine::physics {

struct RaycastHit {
  engine::math::Vec3 position{0.0f};
  bool hit = false;
};

class PhysicsSystem {
public:
  RaycastHit raycast(const engine::math::Vec3& origin, const engine::math::Vec3& direction) const;
};

}  // namespace engine::physics
