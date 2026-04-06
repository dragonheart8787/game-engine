#include "weavebound/renderer/forward_plus_phase1.hpp"

#include "weavebound/ecs/registry.hpp"
#include "weavebound/ecs/scene_types.hpp"

#include <algorithm>
#include <cmath>

namespace weavebound::renderer {

namespace {

void mat_look(float eye[3], float at[3], float upv[3], float o[16]) {
  float f[3] = {at[0] - eye[0], at[1] - eye[1], at[2] - eye[2]};
  float fl = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
  if (fl > 1e-6f) {
    f[0] /= fl;
    f[1] /= fl;
    f[2] /= fl;
  }
  float s[3] = {f[1] * upv[2] - f[2] * upv[1], f[2] * upv[0] - f[0] * upv[2], f[0] * upv[1] - f[1] * upv[0]};
  fl = std::sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
  if (fl > 1e-6f) {
    s[0] /= fl;
    s[1] /= fl;
    s[2] /= fl;
  }
  float u[3] = {s[1] * f[2] - s[2] * f[1], s[2] * f[0] - s[0] * f[2], s[0] * f[1] - s[1] * f[0]};
  o[0] = s[0];
  o[1] = u[0];
  o[2] = -f[0];
  o[3] = 0;
  o[4] = s[1];
  o[5] = u[1];
  o[6] = -f[1];
  o[7] = 0;
  o[8] = s[2];
  o[9] = u[2];
  o[10] = -f[2];
  o[11] = 0;
  o[12] = -(s[0] * eye[0] + s[1] * eye[1] + s[2] * eye[2]);
  o[13] = -(u[0] * eye[0] + u[1] * eye[1] + u[2] * eye[2]);
  o[14] = f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2];
  o[15] = 1.f;
}

void mat4_perspective(float fov_y_rad, float aspect, float n, float f, float* out_col16) {
  const float t = n * std::tan(fov_y_rad * 0.5f);
  const float r = t * aspect;
  const float l = -r;
  const float b = -t;
  for (int i = 0; i < 16; ++i) {
    out_col16[i] = 0.f;
  }
  out_col16[0] = (2.f * n) / (r - l);
  out_col16[5] = (2.f * n) / (t - b);
  out_col16[10] = -(f + n) / (f - n);
  out_col16[11] = -1.f;
  out_col16[14] = -(2.f * f * n) / (f - n);
}

void mat4_mul_col(const float* a, const float* b, float* out_col16) {
  for (int c = 0; c < 4; ++c) {
    for (int r = 0; r < 4; ++r) {
      out_col16[c * 4 + r] = a[0 * 4 + r] * b[c * 4 + 0] + a[1 * 4 + r] * b[c * 4 + 1] +
                             a[2 * 4 + r] * b[c * 4 + 2] + a[3 * 4 + r] * b[c * 4 + 3];
    }
  }
}

}  // namespace

void ForwardPlusPhase1::render_frame() {
  visible_ = total_ = 0;
  ecs::Entity cam_e{};
  bool have_cam = false;
  world_.for_each_living_with_transform([&](ecs::Entity e, const ecs::Transform3&) {
    if (world_.has_camera(e) && !have_cam) {
      cam_e = e;
      have_cam = true;
    }
  });
  if (!have_cam) {
    return;
  }

  const ecs::Transform3& tr = world_.transform(cam_e);
  const ecs::Camera& cam = world_.camera_of(cam_e);

  /** Phase1：視角方向由「相機位置 → 固定注視點」決定（與 lit 滑鼠軌道相機同步時請每幀更新 transform.position）。 */
  float eye[3] = {tr.position[0], tr.position[1], tr.position[2]};
  float at[3] = {0.f, 0.4f, 0.f};
  float upv[3] = {0.f, 1.f, 0.f};

  float view[16]{};
  mat_look(eye, at, upv, view);

  float proj[16]{};
  const float aspect =
      (viewport_w_ > 0 && viewport_h_ > 0)
          ? static_cast<float>(viewport_w_) / static_cast<float>(std::max(1, viewport_h_))
          : (16.f / 9.f);
  mat4_perspective(cam.fov_y_deg * 3.14159265f / 180.f, aspect, cam.near_z, cam.far_z, proj);

  float vp[16]{};
  mat4_mul_col(proj, view, vp);
  const Frustum fr = frustum_from_view_proj(vp);

  world_.for_each_entity_with_aabb([&](ecs::Entity e, const ecs::Aabb& box) {
    if (!world_.has_mesh_renderer(e)) {
      return;
    }
    ++total_;
    Aabb rb{};
    for (int k = 0; k < 3; ++k) {
      rb.min[k] = box.min[k];
      rb.max[k] = box.max[k];
    }
    if (aabb_visible_frustum(fr, rb)) {
      ++visible_;
    }
  });
}

}  // namespace weavebound::renderer
