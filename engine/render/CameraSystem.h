#pragma once

#include "engine/math/Math.h"

namespace engine::render {

class CameraSystem {
public:
  void setView(const engine::math::Vec3& position, const engine::math::Vec3& target);
  void setPerspective(float fovRadians, float aspect, float nearPlane, float farPlane);

  engine::math::Mat4 viewProj() const { return projection_ * view_; }

private:
  engine::math::Mat4 view_ = engine::math::Mat4(1.0f);
  engine::math::Mat4 projection_ = engine::math::Mat4(1.0f);
};

}  // namespace engine::render
