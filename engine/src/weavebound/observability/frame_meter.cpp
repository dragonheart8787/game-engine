#include "weavebound/observability/frame_meter.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace weavebound::observability {

namespace {

constexpr int kCap = 128;
float g_buf[kCap]{};
int g_count{0};
int g_pos{0};

}  // namespace

void record_frame_dt_seconds(float dt_seconds) {
  const float ms = dt_seconds * 1000.f;
  g_buf[g_pos] = ms;
  g_pos = (g_pos + 1) % kCap;
  g_count = std::min(kCap, g_count + 1);
}

float frame_time_avg_ms_last_n(int n) {
  if (g_count <= 0) {
    return 0.f;
  }
  n = std::max(1, std::min(n, g_count));
  float sum = 0.f;
  for (int i = 0; i < n; ++i) {
    const int idx = (g_pos - 1 - i + kCap) % kCap;
    sum += g_buf[idx];
  }
  return sum / static_cast<float>(n);
}

float frame_time_max_ms_last_n(int n) {
  if (g_count <= 0) {
    return 0.f;
  }
  n = std::max(1, std::min(n, g_count));
  float mx = 0.f;
  for (int i = 0; i < n; ++i) {
    const int idx = (g_pos - 1 - i + kCap) % kCap;
    mx = std::max(mx, g_buf[idx]);
  }
  return mx;
}

}  // namespace weavebound::observability
