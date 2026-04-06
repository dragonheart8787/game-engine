#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

#include "game_engine/core.hpp"
#include "weavebound/ecs/scene_types.hpp"
#include "weavebound/rhi/lit_demo_frame.hpp"
#include "weavebound/renderer/forward_plus_phase1.hpp"
#include "weavebound/weavebound.hpp"

int main() {
  using namespace weavebound;

  std::cout << "WeaveBound renderer_demo (" << version::string() << ")\n";

  engine::ApplicationConfig cfg;
  cfg.title = "WeaveBound Renderer Demo (M2)";
  cfg.width_px = 1280;
  cfg.height_px = 720;
  cfg.enable_lit_demo = true;
  cfg.request_vulkan = true;

  engine::Application app;
  if (!app.startup(cfg)) {
    std::cout << "startup failed\n";
    return 1;
  }

  rhi::IDevice* dev = app.device();
  if (!dev || !dev->is_valid()) {
    std::cout << "Vulkan device not available (install SDK / GPU)\n";
    return 0;
  }

  ecs::Entity cam = app.world().spawn();
  app.world().set_camera(cam, ecs::Camera{});
  app.world().transform(cam).position[0] = 0.f;
  app.world().transform(cam).position[1] = 2.6f;
  app.world().transform(cam).position[2] = 4.f;

  ecs::Entity mesh_ent = app.world().spawn();
  app.world().set_mesh_renderer(mesh_ent, ecs::MeshRenderer{1});
  ecs::Aabb box{};
  box.min[0] = box.min[1] = box.min[2] = -0.5f;
  box.max[0] = box.max[1] = box.max[2] = 0.5f;
  app.world().set_aabb(mesh_ent, box);

  renderer::ForwardPlusPhase1 forward_plus(app.world(), cfg.width_px, cfg.height_px);

  int max_frames = 10'000;
  if (const char* fs = std::getenv("WEAVEBOUND_SMOKE_FRAMES")) {
    const int v = std::atoi(fs);
    if (v >= 0) {
      max_frames = (v < 1'000'000) ? v : 1'000'000;
    }
  }

  float time_acc = 0.f;
  int frame = 0;
  double fps_smooth = 0.0;
  float yaw = 0.f;
  float pitch = 0.f;
#if defined(_WIN32)
  int last_mx = 0;
  int last_my = 0;
  bool have_mouse_ref = false;
#endif

  while (frame < max_frames) {
    float dt = 0.f;
    if (!app.tick(&dt)) {
      break;
    }
    time_acc += dt;
    if (dt > 1e-6f) {
      const double inst = 1.0 / static_cast<double>(dt);
      fps_smooth = (fps_smooth <= 0.0) ? inst : fps_smooth * 0.92 + inst * 0.08;
    }

    if (app.window()) {
      forward_plus.set_viewport_pixels(app.window()->width(), app.window()->height());
    }

#if defined(_WIN32)
    if (app.window()) {
      HWND hwnd = static_cast<HWND>(app.window()->native_window_handle());
      POINT pt{};
      GetCursorPos(&pt);
      ScreenToClient(hwnd, &pt);
      if (have_mouse_ref) {
        yaw += static_cast<float>(pt.x - last_mx) * 0.005f;
        pitch += static_cast<float>(last_my - pt.y) * 0.005f;
      }
      last_mx = pt.x;
      last_my = pt.y;
      have_mouse_ref = true;
      pitch = std::clamp(pitch, -1.4f, 1.4f);

      constexpr float k_la[3] = {0.f, 0.4f, 0.f};
      constexpr float k_dist = 4.f;
      const float cy = std::cos(yaw);
      const float sy = std::sin(yaw);
      const float cp = std::cos(pitch);
      const float sp = std::sin(pitch);
      ecs::Transform3& ct = app.world().transform(cam);
      ct.position[0] = k_la[0] + cy * cp * k_dist;
      ct.position[1] = k_la[1] + sp * k_dist;
      ct.position[2] = k_la[2] + sy * cp * k_dist;
    }
#endif

    forward_plus.render_frame();

    ImGui_ImplWin32_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("WeaveBound");
    ImGui::Text("fps %.1f  dt %.2f ms", fps_smooth, static_cast<double>(dt * 1000.f));
    ImGui::Text("cull visible %zu / total %zu", forward_plus.last_visible_draws(),
                forward_plus.last_total_meshes());
    ImGui::End();
    ImGui::Render();

    rhi::LitDemoFrameParams lit{};
    lit.imgui_draw_data = ImGui::GetDrawData();
    lit.demo_time_seconds = time_acc;
    lit.use_mouse_camera = true;
    lit.yaw_rad = yaw;
    lit.pitch_rad = pitch;
    lit.orbit_distance = 4.f;
    lit.look_at[0] = 0.f;
    lit.look_at[1] = 0.4f;
    lit.look_at[2] = 0.f;

#if defined(_WIN32)
    if (app.window() && (frame % 30 == 0)) {
      wchar_t title[256];
      swprintf_s(title, L"WeaveBound M2 | %.0f fps | cull %zu/%zu", fps_smooth,
                 forward_plus.last_visible_draws(), forward_plus.last_total_meshes());
      SetWindowTextW(static_cast<HWND>(app.window()->native_window_handle()), title);
    }
#endif

    if (!dev->clear_present_rgba(0.04f, 0.05f, 0.08f, 1.f, nullptr, &lit)) {
      std::cout << "clear_present failed at frame " << frame << '\n';
      break;
    }
    ++frame;
#if defined(_WIN32)
    Sleep(1);
#endif
  }

  std::cout << "frames=" << frame << " time_acc=" << time_acc << '\n';
  return 0;
}
