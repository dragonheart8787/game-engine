#pragma once

namespace weavebound::rhi {

/** 每幀傳入 Lit／Vulkan 示範路徑的選項（ImGui、時間軌道相機或滑鼠相機）。 */
struct LitDemoFrameParams {
  void* imgui_draw_data{};
  float demo_time_seconds{};
  /** 為真時以 yaw/pitch／orbit 繞 look_at 建視角，忽略時間軌道。 */
  bool use_mouse_camera{};
  float yaw_rad{};
  float pitch_rad{};
  float orbit_distance{4.f};
  float look_at[3]{0.f, 0.4f, 0.f};
  /** 加在 lit 立方體變換上（欄 12–14 平移）；預設零。prototype 用平面座標映射。 */
  float lit_cube_translate[3]{0.f, 0.f, 0.f};
};

}  // namespace weavebound::rhi
