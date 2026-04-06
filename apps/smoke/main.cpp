#include <cstdlib>
#include <iostream>
#include <algorithm>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

#include "game_engine/core.hpp"
#include "weavebound/renderer/forward_plus_phase1.hpp"
#include "weavebound/weavebound.hpp"

static bool env_truthy(const char* v) {
  if (!v || !*v) {
    return false;
  }
  return v[0] == '1' || v[0] == 'y' || v[0] == 'Y';
}

int main() {
  using namespace weavebound;

  std::cout << "WeaveBound " << version::string() << " (" << version::kMilestone << ")\n";
  std::cout << "game_engine_core major=" << game_engine::core_version_major() << '\n';

  engine::ApplicationConfig cfg;
  cfg.title = "WeaveBound Smoke";
  cfg.width_px = 800;
  cfg.height_px = 600;
  cfg.vulkan_debug_layers = env_truthy(std::getenv("WEAVEBOUND_VK_DEBUG"));

  engine::Application app;
  if (!app.startup(cfg)) {
    std::cout << "Application::startup failed\n";
    return 1;
  }

  auto clock = platform::create_std_clock();
  std::cout << "clock elapsed_s=" << clock->elapsed_seconds() << '\n';

  if (app.window()) {
    std::cout << "window " << app.window()->width() << 'x' << app.window()->height()
              << " open=" << app.window()->is_open() << '\n';
  }

  auto jobs = platform::create_inline_job_system();
  int n = 0;
  jobs->submit([&n] { ++n; });
  jobs->wait_all();
  std::cout << "inline job n=" << n << '\n';

  rg::RenderGraph graph;
  graph.add_pass(rg::PassKind::Graphics);
  graph.add_pass(rg::PassKind::Compute);
  std::cout << "render_graph passes=" << graph.pass_count() << '\n';

  rg::RenderGraphBuilder rg_b;
  const auto rt_a = rg_b.add_resource("offscreen_a", true);
  const auto rt_b = rg_b.add_resource("offscreen_b", true);
  const auto bb = rg_b.add_resource("swapchain", false);
  const auto p_clear = rg_b.add_pass("clear", rg::PassKind::Graphics);
  rg_b.pass_writes(p_clear, rt_a);
  const auto p_off = rg_b.add_pass("offscreen", rg::PassKind::Graphics);
  rg_b.pass_reads(p_off, rt_a);
  rg_b.pass_writes(p_off, rt_b);
  const auto p_comp = rg_b.add_pass("composite", rg::PassKind::Graphics);
  rg_b.pass_reads(p_comp, rt_b);
  rg_b.pass_writes(p_comp, bb);
  const rg::CompiledRenderGraph compiled = rg::compile(rg_b);
  std::cout << "render_graph compile_ok=" << compiled.ok << " order_size=" << compiled.pass_order.size()
            << " barriers=" << compiled.barriers.size() << '\n';
  if (compiled.ok) {
    std::cout << "render_graph json_bytes=" << compiled.dump_json().size() << '\n';
  }

  rg::RenderGraphBuilder rg_depth;
  const auto d_tex = rg_depth.add_resource("depth_tex", true, rg::ResourceSurfaceKind::Depth);
  const auto p_depth_write = rg_depth.add_pass("depth_pass", rg::PassKind::Graphics);
  rg_depth.pass_writes(p_depth_write, d_tex);
  const auto p_depth_read = rg_depth.add_pass("sample_depth", rg::PassKind::Graphics);
  rg_depth.pass_reads(p_depth_read, d_tex);
  const rg::CompiledRenderGraph cd = rg::compile(rg_depth);
  if (!cd.ok || cd.barriers.size() != 1u || cd.barriers[0].note != "depth_attachment_to_srv") {
    std::cout << "depth render_graph compile failed or unexpected barrier\n";
    return 1;
  }

  ecs::Entity e = app.world().spawn();
  app.world().transform(e).position[0] = 1.f;
  ecs::Aabb box{};
  box.min[0] = box.min[1] = box.min[2] = -0.5f;
  box.max[0] = box.max[1] = box.max[2] = 0.5f;
  app.world().set_aabb(e, box);
  app.world().set_mesh_renderer(e, ecs::MeshRenderer{1});
  app.world().set_camera(e, ecs::Camera{});
  renderer::ForwardPlusPhase1 phase1_cull(app.world(), app.window() ? app.window()->width() : 800,
                                          app.window() ? app.window()->height() : 600);
  phase1_cull.render_frame();
  std::cout << "ecs spawn entity index=" << e.index << " gen=" << e.generation
            << " living=" << app.world().living_count() << '\n';

  rhi::IDevice* dev = app.device();
  std::cout << "rhi device valid=" << (dev && dev->is_valid()) << '\n';

#if defined(_WIN32)
  if (dev && dev->is_valid()) {
    int max_frames = 120;
    if (const char* fs = std::getenv("WEAVEBOUND_SMOKE_FRAMES")) {
      const int v = std::atoi(fs);
      if (v >= 0) {
        max_frames = std::min(v, 100000);
      }
    }
    std::cout << "clear_present loop max_frames=" << max_frames
              << " (set WEAVEBOUND_SMOKE_FRAMES, ESC closes)…\n";
    int frame = 0;
    while (frame < max_frames) {
      float dt = 0.f;
      if (!app.tick(&dt)) {
        break;
      }
      const float p = static_cast<float>(frame % 400) / 400.f;
      if (!dev->clear_present_rgba(0.08f + 0.2f * p, 0.12f, 0.2f + 0.25f * (1.f - p), 1.f, &compiled,
                                   nullptr)) {
        std::cout << "clear_present failed at frame " << frame << '\n';
        break;
      }
      ++frame;
      Sleep(8);
    }
  }
#else
  float dt = 0.f;
  app.tick(&dt);
#endif

  app.world().destroy(e);
  std::cout << "net INetDriver (placeholder only)\n";
  return 0;
}
