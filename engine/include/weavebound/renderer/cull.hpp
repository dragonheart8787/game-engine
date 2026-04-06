#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace weavebound::renderer {

/** 視錐 6 平面：平面方程式 n·x + d <= 0 為內側（左手座標常見約定）。 */
struct Frustum {
  std::array<float, 4> planes[6]{};
};

struct Aabb {
  float min[3]{};
  float max[3]{};
};

/** 由 column-major view-projection 矩陣建立視錐（與 GL 風格 clip 空間相容）。 */
inline Frustum frustum_from_view_proj(const float* mvp_col16) {
  Frustum f{};
  const auto row = [&](int r, int c) { return mvp_col16[c * 4 + r]; };
  for (int i = 0; i < 4; ++i) {
    f.planes[0][i] = row(3, i) + row(0, i);
    f.planes[1][i] = row(3, i) - row(0, i);
    f.planes[2][i] = row(3, i) + row(1, i);
    f.planes[3][i] = row(3, i) - row(1, i);
    f.planes[4][i] = row(3, i) + row(2, i);
    f.planes[5][i] = row(3, i) - row(2, i);
  }
  for (auto& pl : f.planes) {
    const float len = std::sqrt(pl[0] * pl[0] + pl[1] * pl[1] + pl[2] * pl[2]);
    if (len > 1e-8f) {
      for (float& c : pl) {
        c /= len;
      }
    }
  }
  return f;
}

inline float plane_distance(const std::array<float, 4>& pl, float x, float y, float z) {
  return pl[0] * x + pl[1] * y + pl[2] * z + pl[3];
}

/** AABB 頂點 p 若滿足所有平面 n·p+d <= 0 則在錐內；使用「任一頂點在平面正側則剔除」的保守測試。 */
inline bool aabb_visible_frustum(const Frustum& f, const Aabb& box) {
  const float corners[8][3] = {
      {box.min[0], box.min[1], box.min[2]}, {box.max[0], box.min[1], box.min[2]},
      {box.min[0], box.max[1], box.min[2]}, {box.max[0], box.max[1], box.min[2]},
      {box.min[0], box.min[1], box.max[2]}, {box.max[0], box.min[1], box.max[2]},
      {box.min[0], box.max[1], box.max[2]}, {box.max[0], box.max[1], box.max[2]},
  };
  for (std::size_t pi = 0; pi < 6; ++pi) {
    bool all_out = true;
    for (const auto& c : corners) {
      if (plane_distance(f.planes[pi], c[0], c[1], c[2]) <= 0.f) {
        all_out = false;
        break;
      }
    }
    if (all_out) {
      return false;
    }
  }
  return true;
}

}  // namespace weavebound::renderer
