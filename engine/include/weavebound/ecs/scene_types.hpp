#pragma once

#include <cstdint>

namespace weavebound::ecs {

using EntityId = std::uint32_t;

/** Hybrid ECS 占位：之後改為 SoA storage 與 system scheduler（規格 §1.4）。 */
struct Transform3 {
  float position[3]{};
  float rotation_quat[4]{0, 0, 0, 1};
  float scale[3]{1, 1, 1};
};

struct Aabb {
  float min[3]{-1.f, -1.f, -1.f};
  float max[3]{1.f, 1.f, 1.f};
};

/** 內建 mesh 識別（cook 資產接上後改為 asset id）。 */
struct MeshRenderer {
  std::uint32_t mesh_id{0};
};

struct Camera {
  float fov_y_deg{60.f};
  float near_z{0.05f};
  float far_z{256.f};
};

struct DirectionalLight {
  float direction[3]{0.35f, -0.85f, 0.25f};
  float radiance[3]{1.f, 0.98f, 0.95f};
};

}  // namespace weavebound::ecs
