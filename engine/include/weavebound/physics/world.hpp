#pragma once

#include <cstdint>

namespace weavebound::physics {

using BodyId = std::uint32_t;

/** Jolt/PhysX/Bullet 整合占位（規格 1.7）。 */
class IPhysicsWorld {
public:
  virtual ~IPhysicsWorld() = default;
  virtual void step_fixed(double dt) = 0;
};

}  // namespace weavebound::physics
