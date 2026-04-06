#include "game_flow.hpp"

namespace weavebound::game_prototype {

void GameFlow::update(float dt_seconds) {
  ++frames_;
  switch (state_) {
    case GameState::Boot:
      boot_timer_ += dt_seconds;
      if (boot_timer_ >= 0.05f) {
        state_ = GameState::MainMenu;
        boot_timer_ = 0.f;
      }
      break;
    case GameState::Loading:
      load_timer_ += dt_seconds;
      if (load_timer_ >= 0.05f) {
        state_ = GameState::InGame;
        load_timer_ = 0.f;
      }
      break;
    case GameState::MainMenu:
    case GameState::InGame:
    case GameState::Paused:
    case GameState::GameOver:
    case GameState::Victory:
      break;
  }
}

void GameFlow::request_new_game() {
  if (state_ == GameState::MainMenu) {
    pending_scene_path_ = "scenes/placeholder.wbs";
    state_ = GameState::Loading;
    load_timer_ = 0.f;
  }
}

void GameFlow::request_load_slot0() {
  if (state_ == GameState::MainMenu) {
    pending_scene_path_ = "scenes/placeholder.wbs";
    state_ = GameState::Loading;
    load_timer_ = 0.f;
  }
}

void GameFlow::request_pause() {
  if (state_ == GameState::InGame) {
    state_ = GameState::Paused;
  }
}

void GameFlow::request_resume() {
  if (state_ == GameState::Paused) {
    state_ = GameState::InGame;
  }
}

void GameFlow::request_quit_to_menu() {
  if (state_ == GameState::InGame || state_ == GameState::Paused) {
    state_ = GameState::MainMenu;
    pending_scene_path_.clear();
  }
}

void GameFlow::notify_victory() {
  if (state_ == GameState::InGame) {
    state_ = GameState::Victory;
  }
}

void GameFlow::notify_game_over() {
  if (state_ == GameState::InGame) {
    state_ = GameState::GameOver;
  }
}

void GameFlow::acknowledge_end() {
  if (state_ == GameState::Victory || state_ == GameState::GameOver) {
    state_ = GameState::MainMenu;
    pending_scene_path_.clear();
  }
}

void GameFlow::debug_advance_after_frames(int n) {
  if (frames_ >= n && state_ == GameState::MainMenu) {
    request_new_game();
  }
}

}  // namespace weavebound::game_prototype
