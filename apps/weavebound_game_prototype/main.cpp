// 可執行檔模式：預設 CI（勝利+暫停迴歸）；--play 互動；--ci-gameover 敗北迴歸；--save-smoke 僅存檔測試；
// --ability-smoke 能力切片；--settings-smoke / --m1-flow-smoke / --economy-smoke；建置含 Vulkan 時 --play 即使用 lit+ImGui HUD。
#include "ability_spec_loader.hpp"
#include "action_map.hpp"
#include "game_flow.hpp"
#include "level_spec.hpp"
#include "play_session.hpp"
#include "prototype_settings.hpp"
#include "prototype_sound.hpp"

#include "weavebound/engine/application.hpp"
#include "weavebound/game/save_game_v0.hpp"
#include "weavebound/observability/profiler.hpp"
#include "weavebound/platform/input.hpp"

#include <cstdio>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(WB_GAME_PROTOTYPE_HAS_VULKAN)
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_win32.h>

#include "weavebound/rhi/lit_demo_frame.hpp"
#endif

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static constexpr const char* kSlotPath = "weavebound_prototype_slot0.wbsv";

static bool arg_is(int argc, char** argv, const char* flag) {
  return argc > 1 && std::strcmp(argv[1], flag) == 0;
}

static bool arg_any(int argc, char** argv, const char* flag) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], flag) == 0) {
      return true;
    }
  }
  return false;
}

static int run_save_smoke() {
  using namespace weavebound::game::save_v0;
  PlayerChunk p{};
  p.health = 100.f;
  p.level = 1;
  const auto bytes = encode(p);
  const auto back = decode(bytes);
  if (!back || back->player.level != 1u) {
    return 1;
  }
  std::cout << "save_v0 roundtrip ok bytes=" << bytes.size() << '\n';
  return 0;
}

static bool write_slot0(const weavebound::game::save_v0::PlayerChunk& player,
                        const weavebound::game::save_v0::WorldFlagsChunk& world,
                        const weavebound::game::save_v0::QuestChunk* quest) {
  using namespace weavebound::game::save_v0;
  const auto bytes = encode(player, world, quest);
  std::ofstream f(kSlotPath, std::ios::binary);
  if (!f) {
    return false;
  }
  f.write(reinterpret_cast<const char*>(bytes.data()),
          static_cast<std::streamsize>(bytes.size()));
  return static_cast<bool>(f);
}

static bool read_slot0(weavebound::game::save_v0::DecodedSave& out) {
  std::ifstream f(kSlotPath, std::ios::binary | std::ios::ate);
  if (!f) {
    return false;
  }
  const auto sz = f.tellg();
  if (sz <= 0) {
    return false;
  }
  f.seekg(0);
  std::vector<std::uint8_t> buf(static_cast<std::size_t>(sz));
  f.read(reinterpret_cast<char*>(buf.data()), sz);
  const auto dec = weavebound::game::save_v0::decode(buf);
  if (!dec) {
    return false;
  }
  out = *dec;
  return true;
}

#if defined(_WIN32)
static void set_title_bar(weavebound::engine::Application& app, const std::wstring& text) {
  if (!app.window()) {
    return;
  }
  void* hwnd = app.window()->native_window_handle();
  if (hwnd) {
    SetWindowTextW(static_cast<HWND>(hwnd), text.c_str());
  }
}

static bool try_read_ability_json_beside_exe(std::string& out_text) {
  wchar_t wb[2048];
  if (GetModuleFileNameW(nullptr, wb, 2048) == 0) {
    return false;
  }
  std::wstring w(wb);
  const size_t sl = w.find_last_of(L"\\/");
  if (sl != std::wstring::npos) {
    w.resize(sl + 1);
  } else {
    w.clear();
  }
  w += L"ability_slice_v0.json";
  std::ifstream wf(w, std::ios::binary);
  if (!wf) {
    return false;
  }
  std::ostringstream ss;
  ss << wf.rdbuf();
  out_text = ss.str();
  return true;
}
#endif

namespace {

bool edge_down(const weavebound::platform::InputState& cur,
               const weavebound::platform::InputState& prev,
               weavebound::platform::Key k) {
  using weavebound::platform::key_down;
  return key_down(cur, k) && !key_down(prev, k);
}

void configure_session_abilities(weavebound::game_prototype::PlaySession& session, const char* argv0) {
  using namespace weavebound::game_prototype;
  AbilitySliceRuntime spec{};
  std::string err;
  if (load_ability_slice_v0_from_file("ability_slice_v0.json", spec, err)) {
    session.configure_abilities(spec);
    return;
  }
  if (argv0) {
    const std::filesystem::path base(argv0);
    const auto p = base.parent_path() / "ability_slice_v0.json";
    if (load_ability_slice_v0_from_file(p.string(), spec, err)) {
      session.configure_abilities(spec);
      return;
    }
  }
#if defined(_WIN32)
  std::string raw;
  if (try_read_ability_json_beside_exe(raw) && load_ability_slice_v0_from_string(raw, spec, err)) {
    session.configure_abilities(spec);
    return;
  }
#endif
  session.configure_abilities(AbilitySliceRuntime{});
}

void apply_session_level_m1(weavebound::game_prototype::PlaySession& session, const char* argv0) {
  using namespace weavebound::game_prototype;
  LevelM1Spec lv{};
  std::string err;
  bool ok = load_level_m1_from_file("level_m1.json", lv, err);
  if (!ok && argv0) {
    const std::filesystem::path base(argv0);
    ok = load_level_m1_from_file((base.parent_path() / "level_m1.json").string(), lv, err);
  }
#if defined(_WIN32)
  if (!ok) {
    wchar_t wb[2048];
    if (GetModuleFileNameW(nullptr, wb, 2048) != 0) {
      std::wstring w(wb);
      const size_t sl = w.find_last_of(L"\\/");
      if (sl != std::wstring::npos) {
        w.resize(sl + 1);
      } else {
        w.clear();
      }
      w += L"level_m1.json";
      std::ifstream wf(w, std::ios::binary);
      if (wf) {
        std::ostringstream ss;
        ss << wf.rdbuf();
        ok = load_level_m1_from_string(ss.str(), lv, err);
      }
    }
  }
#endif
  if (!ok || lv.phases.size() < 3u) {
    return;
  }
  std::vector<std::string> o;
  std::vector<std::string> h;
  o.reserve(lv.phases.size());
  h.reserve(lv.phases.size());
  for (const auto& ph : lv.phases) {
    o.push_back(ph.objective);
    h.push_back(ph.hint);
  }
  session.set_m1_phase_lines(std::move(o), std::move(h));
}

}  // namespace

static int run_ability_smoke() {
  using namespace weavebound::game_prototype;
  PlaySession session;
  configure_session_abilities(session, nullptr);
  session.debug_clear_threats();
  session.debug_set_position(14.f, 14.f);
  session.debug_spawn_enemy_at(14.f, 16.f, 80.f);
  constexpr float dt = 1.f / 60.f;
  SessionInput in{};
  in.primary_pressed = true;
  session.update(dt, in);
  in.primary_pressed = false;
  for (int i = 0; i < 30; ++i) {
    (void)session.update(dt, in);
  }
  const auto& en = session.enemies();
  if (en.empty() || en[0].hp > 80.f - 5.f) {
    std::cerr << "ability_smoke: expected wedge damage\n";
    return 1;
  }
  std::cout << "ability_smoke ok (enemy hp=" << en[0].hp << ")\n";
  return 0;
}

static int run_settings_smoke() {
  using namespace weavebound::game_prototype;
  PrototypeSettings w{};
  w.master_volume = 0.42f;
  w.sound_enabled = true;
  w.tutorial_dismissed = false;
  std::string err;
  if (!save_prototype_settings(w, err, nullptr)) {
    std::cerr << "settings_smoke save: " << err << '\n';
    return 1;
  }
  PrototypeSettings r{};
  if (!load_prototype_settings(r, err, nullptr)) {
    std::remove("weavebound_prototype_settings.json");
    return 1;
  }
  std::remove("weavebound_prototype_settings.json");
  if (std::fabs(static_cast<double>(r.master_volume - 0.42f)) > 0.02) {
    return 1;
  }
  if (!r.sound_enabled) {
    return 1;
  }
  std::cout << "settings_smoke ok\n";
  return 0;
}

static int run_m1_flow_smoke() {
  using namespace weavebound::game_prototype;
  PlaySession session;
  configure_session_abilities(session, nullptr);
  apply_session_level_m1(session, nullptr);
  session.set_multi_stage_campaign(true);
  session.set_autopilot_to_goal(true);
  session.debug_clear_threats();
  constexpr float dt = 1.f / 60.f;
  const SessionInput in{};
  for (int i = 0; i < 6000; ++i) {
    if (session.update(dt, in) == SessionOutcome::Victory) {
      std::cout << "m1_flow_smoke ok frames=" << i << '\n';
      return 0;
    }
  }
  std::cerr << "m1_flow_smoke: timeout\n";
  return 1;
}

static int run_economy_smoke() {
  using namespace weavebound::game::save_v0;
  using namespace weavebound::game_prototype;
  PlaySession session;
  configure_session_abilities(session, nullptr);
  session.debug_clear_threats();
  session.debug_set_position(14.f, 14.f);
  session.debug_spawn_enemy_at(14.f, 16.f, 5.f);
  constexpr float dt = 1.f / 60.f;
  SessionInput in{};
  in.primary_pressed = true;
  (void)session.update(dt, in);
  in.primary_pressed = false;
  for (int i = 0; i < 120; ++i) {
    (void)session.update(dt, in);
  }
  if (session.prototype_scrap() < 3u) {
    std::cerr << "economy_smoke: expected scrap from kill\n";
    return 1;
  }
  const auto w = session.build_world_flags();
  const auto p = session.build_player_chunk();
  QuestChunk q{};
  q.active_quest_id = 1u;
  q.step_index = 0u;
  const auto blob = encode(p, w, &q);
  const auto dec = decode(blob);
  if (!dec) {
    std::cerr << "economy_smoke: decode failed\n";
    return 1;
  }
  PlaySession session2;
  configure_session_abilities(session2, nullptr);
  session2.apply_save(dec->player);
  session2.apply_world_flags(dec->world);
  if (session2.prototype_scrap() != session.prototype_scrap()) {
    std::cerr << "economy_smoke: scrap roundtrip mismatch\n";
    return 1;
  }
  std::cout << "economy_smoke ok scrap=" << session2.prototype_scrap() << '\n';
  return 0;
}

static int run_ci_smoke(weavebound::engine::Application& app) {
  using namespace weavebound::game_prototype;
  GameFlow flow;
  PlaySession session;
  constexpr float kSimDt = 1.f / 60.f;
  bool leg1_session = false;
  bool leg1_campaign_done = false;
  bool leg2_started = false;
  bool leg2_session = false;
  int leg2_start_f = -1;
  float dt = 0.f;
  const SessionInput kNoInput{};

  for (int f = 0; f < 800; ++f) {
    if (!app.tick(&dt)) {
      return 2;
    }
    flow.update(kSimDt);

    if (f == 6) {
      flow.debug_advance_after_frames(0);
    }

    if (flow.state() == GameState::InGame && !leg2_started) {
      if (!leg1_session) {
        session.reset_new_game();
        session.debug_clear_threats();
        session.set_autopilot_to_goal(true);
        leg1_session = true;
      }
      const SessionOutcome out = session.update(kSimDt, kNoInput);
      if (out == SessionOutcome::Victory) {
        flow.notify_victory();
      }
    }

    if (flow.state() != GameState::InGame && !leg2_started) {
      leg1_session = false;
    }

    if (flow.state() == GameState::Victory) {
      flow.acknowledge_end();
      leg1_campaign_done = true;
    }

    if (leg1_campaign_done && !leg2_started && flow.state() == GameState::MainMenu) {
      flow.request_new_game();
      leg2_started = true;
    }

    if (leg2_started && flow.state() == GameState::InGame) {
      if (!leg2_session) {
        session.reset_new_game();
        session.set_autopilot_to_goal(false);
        leg2_session = true;
        leg2_start_f = f;
      }
      (void)session.update(kSimDt, kNoInput);
    }

    const int leg2_t = (leg2_start_f >= 0) ? (f - leg2_start_f) : -1;

    if (leg2_t >= 0 && flow.state() == GameState::InGame && leg2_t == 12) {
      flow.request_pause();
    }

    if (leg2_t >= 0 && flow.state() == GameState::Paused && leg2_t >= 20) {
      flow.request_resume();
    }

    if (leg2_t >= 0 && flow.state() == GameState::InGame && leg2_t >= 35) {
      flow.request_quit_to_menu();
    }
  }

  if (flow.state() != GameState::MainMenu) {
    std::cerr << "ci_smoke: expected MainMenu\n";
    return 3;
  }
  std::cout << "ci_smoke ok (victory, pause leg, menu)\n";
  return 0;
}

static int run_ci_gameover(weavebound::engine::Application& app) {
  using namespace weavebound::game_prototype;
  GameFlow flow;
  PlaySession session;
  constexpr float kSimDt = 1.f / 60.f;
  bool armed = false;
  float dt = 0.f;
  const SessionInput kNoInput{};
  for (int f = 0; f < 600; ++f) {
    if (!app.tick(&dt)) {
      return 2;
    }
    flow.update(kSimDt);
    if (f == 6) {
      flow.debug_advance_after_frames(0);
    }
    if (flow.state() == GameState::InGame) {
      if (!armed) {
        session.reset_new_game();
        session.debug_set_health(4.f);
        session.debug_set_position(11.f, 22.f);
        session.set_autopilot_to_goal(false);
        armed = true;
      }
      const SessionOutcome out = session.update(kSimDt, kNoInput);
      if (out == SessionOutcome::Defeat) {
        flow.notify_game_over();
      }
    }
    if (flow.state() == GameState::GameOver) {
      flow.acknowledge_end();
      break;
    }
  }
  if (flow.state() != GameState::MainMenu) {
    std::cerr << "ci_gameover: expected MainMenu\n";
    return 3;
  }
  std::cout << "ci_gameover ok\n";
  return 0;
}

static int run_play(weavebound::engine::Application& app, bool vk_present, char* argv0) {
  using namespace weavebound;
  using namespace weavebound::game_prototype;
  GameFlow flow;
  PlaySession session;
  configure_session_abilities(session, argv0);
  apply_session_level_m1(session, argv0);
  PrototypeSettings ui_settings{};
  std::string se;
  load_prototype_settings(ui_settings, se, argv0);
  prototype_sound::g_enabled = ui_settings.sound_enabled;
  prototype_sound::set_master_volume(ui_settings.master_volume);
  platform::InputState prev_in{};
  GameState prev_flow = GameState::Boot;
  std::optional<weavebound::game::save_v0::DecodedSave> pending_apply_load;
  std::optional<weavebound::game::save_v0::QuestChunk> slot_quest_persist;
  constexpr float kSimDt = 1.f / 60.f;
  float dt = 0.f;
  int frame = 0;
  bool debug_spawn_log = false;
  float vk_time_acc = 0.f;

  while (app.tick(&dt)) {
    observability::default_profiler()->begin_zone("prototype_tick");
    const float udt = std::min(dt, 0.1f);
    (void)udt;
    flow.update(kSimDt);

    platform::InputState cur{};
    if (app.window()) {
      app.window()->read_input(cur);
    }

    GameplayActions act = build_gameplay_actions(cur, prev_in);

    const GameState st_mid = flow.state();

    if (prev_flow == GameState::Loading && st_mid == GameState::InGame) {
      if (pending_apply_load) {
        session.reset_new_game();
        session.apply_save(pending_apply_load->player);
        session.apply_world_flags(pending_apply_load->world);
        pending_apply_load.reset();
      } else {
        session.set_multi_stage_campaign(true);
      }
    }

    if (st_mid == GameState::InGame) {
      if (act.pause_pressed) {
        flow.request_pause();
      } else {
        SessionInput sin{};
        sin.move_x = act.move_x;
        sin.move_y = act.move_y;
        sin.dash_pressed = act.dash_pressed;
        sin.primary_pressed = act.primary_pressed;
        sin.consume_pressed = act.consume_scrap_pressed;
        sin.aim_delta_x = static_cast<float>(cur.mouse_dx);
        const SessionOutcome out = session.update(kSimDt, sin);
        if (out == SessionOutcome::Victory) {
          weavebound::prototype_sound::ping_light();
          flow.notify_victory();
        } else if (out == SessionOutcome::Defeat) {
          weavebound::prototype_sound::ping_alert();
          flow.notify_game_over();
        }
        if (edge_down(cur, prev_in, platform::Key::V)) {
          weavebound::prototype_sound::toggle();
          ui_settings.sound_enabled = weavebound::prototype_sound::g_enabled;
        }
      }
      if (debug_spawn_log && (frame % 90) == 0) {
        std::cerr << "[debug_spawn] player " << session.player_x() << "," << session.player_y()
                  << " enemies=" << session.enemies().size() << "\n";
      }
    }

    const GameState st = flow.state();

    if (st == GameState::MainMenu) {
      if (act.confirm_pressed) {
        flow.request_new_game();
      }
      if (edge_down(cur, prev_in, platform::Key::V)) {
        weavebound::prototype_sound::toggle();
        ui_settings.sound_enabled = weavebound::prototype_sound::g_enabled;
      }
      if (edge_down(cur, prev_in, platform::Key::N)) {
        flow.request_new_game();
      }
      if (edge_down(cur, prev_in, platform::Key::L)) {
        weavebound::game::save_v0::DecodedSave loaded{};
        if (read_slot0(loaded)) {
          pending_apply_load = loaded;
          slot_quest_persist.reset();
          if (loaded.has_quest) {
            slot_quest_persist = loaded.quest;
          }
          flow.request_load_slot0();
        }
      }
      if (edge_down(cur, prev_in, platform::Key::K)) {
        debug_spawn_log = !debug_spawn_log;
      }
    }

    if (st == GameState::Paused) {
      if (act.pause_pressed) {
        flow.request_resume();
      }
      if (edge_down(cur, prev_in, platform::Key::V)) {
        weavebound::prototype_sound::toggle();
        ui_settings.sound_enabled = weavebound::prototype_sound::g_enabled;
      }
      if (edge_down(cur, prev_in, platform::Key::K)) {
        const auto chunk = session.build_player_chunk();
        const auto world = session.build_world_flags();
        weavebound::game::save_v0::QuestChunk q{};
        if (slot_quest_persist) {
          q = *slot_quest_persist;
        } else {
          q.active_quest_id = 1u;
          q.step_index = 0u;
        }
        if (write_slot0(chunk, world, &q)) {
          std::cerr << "saved " << kSlotPath << "\n";
        }
      }
      if (edge_down(cur, prev_in, platform::Key::M)) {
        flow.request_quit_to_menu();
      }
    }

    if (st == GameState::Victory || st == GameState::GameOver) {
      if (act.confirm_pressed) {
        flow.acknowledge_end();
      }
    }

#if defined(_WIN32)
    std::wstring title = L"WeaveBound | ";
    switch (st) {
      case GameState::Boot:
        title += L"Boot";
        break;
      case GameState::MainMenu:
        title += L"Menu [Space/N]New [L]Load [K]log [V]sound";
        break;
      case GameState::Loading:
        title += L"Loading";
        break;
      case GameState::InGame: {
        std::string h = session.hud_line();
        title += std::wstring(h.begin(), h.end());
        break;
      }
      case GameState::Paused:
        title += L"Paused [P]Resume [K]Save [M]Menu [V]sound";
        break;
      case GameState::Victory:
        title += L"Victory [Space]OK";
        break;
      case GameState::GameOver:
        title += L"GameOver [Space]OK";
        break;
    }
    set_title_bar(app, title);
#endif

#if defined(WB_GAME_PROTOTYPE_HAS_VULKAN)
    if (vk_present) {
      vk_time_acc += dt;
      weavebound::rhi::IDevice* dev = app.device();
      if (dev && dev->is_valid()) {
        ImGui_ImplWin32_NewFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(16.f, 16.f), ImGuiCond_FirstUseEver);
        ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("WeaveBound M1 | slice + level_m1");
        ImGui::Separator();
        ImGui::Text("HP %.0f | Focus %.0f", static_cast<double>(session.player_health()),
                    static_cast<double>(session.player_focus()));
        {
          const std::string obj = session.objective_line();
          ImGui::TextUnformatted("Objective:");
          ImGui::TextWrapped("%s", obj.c_str());
          if (!ui_settings.tutorial_dismissed) {
            const std::string hi = session.hint_line();
            ImGui::TextUnformatted("Hint:");
            ImGui::TextWrapped("%s", hi.c_str());
          }
        }
        ImGui::End();

        if (st == GameState::Paused) {
          ImGui::SetNextWindowPos(ImVec2(200.f, 160.f), ImGuiCond_FirstUseEver);
          ImGui::Begin("Pause / Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
          ImGui::SliderFloat("Master volume", &ui_settings.master_volume, 0.f, 1.f);
          weavebound::prototype_sound::set_master_volume(ui_settings.master_volume);
          if (ImGui::Checkbox("Sound enabled", &ui_settings.sound_enabled)) {
            weavebound::prototype_sound::g_enabled = ui_settings.sound_enabled;
          }
          ImGui::Checkbox("Dismiss tutorial hint", &ui_settings.tutorial_dismissed);
          ImGui::TextUnformatted(
              "Keybind (stub): WASD move | E primary | Shift dash | C scrap->Focus | P pause | V sfx");
          if (ImGui::Button("Save settings to JSON")) {
            std::string err_w;
            if (save_prototype_settings(ui_settings, err_w, argv0)) {
              ImGui::TextUnformatted("Saved.");
            } else {
              ImGui::Text("Error: %s", err_w.c_str());
            }
          }
          ImGui::End();
        }

        ImGui::Render();

        weavebound::rhi::LitDemoFrameParams lit{};
        lit.imgui_draw_data = ImGui::GetDrawData();
        lit.demo_time_seconds = vk_time_acc;
        lit.use_mouse_camera = true;
        lit.yaw_rad = session.player_facing_yaw();
        lit.pitch_rad = 0.35f;
        lit.orbit_distance = 4.f;
        session.player_lit_look_at(lit.look_at);
        session.player_lit_cube_translate(lit.lit_cube_translate);
        if (!dev->clear_present_rgba(0.04f, 0.05f, 0.08f, 1.f, nullptr, &lit)) {
          std::cerr << "play: clear_present failed\n";
        }
      }
    }
#else
    (void)vk_present;
#endif

    observability::default_profiler()->end_zone();
    prev_in = cur;
    prev_flow = st;
    ++frame;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return 0;
}

int main(int argc, char** argv) {
  if (arg_any(argc, argv, "--save-smoke")) {
    return run_save_smoke();
  }
  if (arg_any(argc, argv, "--ability-smoke")) {
    return run_ability_smoke();
  }
  if (arg_any(argc, argv, "--settings-smoke")) {
    return run_settings_smoke();
  }
  if (arg_any(argc, argv, "--m1-flow-smoke")) {
    return run_m1_flow_smoke();
  }
  if (arg_any(argc, argv, "--economy-smoke")) {
    return run_economy_smoke();
  }

  using namespace weavebound;
  engine::ApplicationConfig cfg;
  cfg.title = "WeaveBound Game Prototype";
  cfg.request_vulkan = false;
  cfg.enable_lit_demo = false;

  const bool play_vk_flag = arg_any(argc, argv, "--play-vk");
#if !defined(WB_GAME_PROTOTYPE_HAS_VULKAN)
  if (play_vk_flag) {
    std::cerr << "--play-vk: this build has no Vulkan lit target (shader/toolchain).\n";
    return 1;
  }
#endif
  const bool play_mode = arg_is(argc, argv, "--play") || play_vk_flag;
  const bool ci_gameover = arg_is(argc, argv, "--ci-gameover");

#if defined(WB_GAME_PROTOTYPE_HAS_VULKAN)
  const bool use_vk_play = play_mode;
#else
  const bool use_vk_play = false;
#endif

  if (play_mode) {
    cfg.visible = true;
    cfg.width_px = 960;
    cfg.height_px = 540;
  } else {
    cfg.visible = false;
  }

  if (use_vk_play) {
    cfg.request_vulkan = true;
    cfg.enable_lit_demo = true;
  }

  engine::Application app;
  if (!app.startup(cfg)) {
    std::cerr << "Application::startup failed\n";
    return 1;
  }

  if (play_mode) {
    return run_play(app, use_vk_play, argc > 0 ? argv[0] : nullptr);
  }
  if (ci_gameover) {
    return run_ci_gameover(app);
  }
  return run_ci_smoke(app);
}
