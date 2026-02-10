#pragma once

#include <cstdint>

#include "engine/math/Math.h"

namespace engine::render {

enum class CameraControlMask : std::uint32_t {
  Follow = 1 << 0,
  FreeLook = 1 << 1,
  CinematicLock = 1 << 2
};

class CameraSystem {
public:
  void setView(const engine::math::Vec3& position, const engine::math::Vec3& target);
  void setPerspective(float fovRadians, float aspect, float nearPlane, float farPlane);
  void setControlMask(CameraControlMask mask) { controlMask_ = mask; }

  engine::math::Mat4 viewProj() const { return projection_ * view_; }
  CameraControlMask controlMask() const { return controlMask_; }

private:
  engine::math::Mat4 view_ = engine::math::Mat4(1.0f);
  engine::math::Mat4 projection_ = engine::math::Mat4(1.0f);
  CameraControlMask controlMask_ = CameraControlMask::Follow;
};

}  // namespace engine::render
