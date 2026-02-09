#include "engine/render/CameraSystem.h"

#include <glm/gtc/matrix_transform.hpp>

namespace engine::render {

void CameraSystem::setView(const engine::math::Vec3& position, const engine::math::Vec3& target) {
  view_ = glm::lookAt(position, target, engine::math::Vec3(0.0f, 1.0f, 0.0f));
}

void CameraSystem::setPerspective(float fovRadians, float aspect, float nearPlane, float farPlane) {
  projection_ = glm::perspective(fovRadians, aspect, nearPlane, farPlane);
}

}  // namespace engine::render
