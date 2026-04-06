#pragma once

#include <string>

namespace weavebound::game_prototype {

/** 可玩原型狀態機（對齊 docs/game/TDD.md §2）。 */
enum class GameState {
  Boot,
  MainMenu,
  Loading,
  InGame,
  Paused,
  GameOver,
  Victory,
};

class GameFlow {
 public:
  GameState state() const { return state_; }

  /** 每幀呼叫（自 Application::tick 取得 dt 後）。 */
  void update(float dt_seconds);

  /** 占位：非同步場景路徑（實際載入接 Job/VFS）。 */
  const std::string& pending_scene_path() const { return pending_scene_path_; }

  void request_new_game();
  void request_load_slot0();
  void request_pause();
  void request_resume();
  void request_quit_to_menu();
  void notify_victory();
  void notify_game_over();
  /** Victory／GameOver 結算後回主選單。 */
  void acknowledge_end();
  /** 測試／自動化：強制推進狀態 */
  void debug_advance_after_frames(int n);

 private:
  GameState state_{GameState::Boot};
  float boot_timer_{0.f};
  float load_timer_{0.f};
  int frames_{0};
  std::string pending_scene_path_;
};

}  // namespace weavebound::game_prototype
