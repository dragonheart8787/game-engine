#include "action_map.hpp"

#include <cmath>

namespace weavebound::game_prototype {

namespace {
using weavebound::platform::InputState;
using weavebound::platform::Key;
using weavebound::platform::key_down;

bool edge_down(const InputState& cur, const InputState& prev, Key k) {
  return key_down(cur, k) && !key_down(prev, k);
}
}  // namespace

GameplayActions build_gameplay_actions(const InputState& cur, const InputState& prev) {
  GameplayActions a{};
  float mx = 0.f;
  float my = 0.f;
  if (key_down(cur, Key::D)) {
    mx += 1.f;
  }
  if (key_down(cur, Key::A)) {
    mx -= 1.f;
  }
  if (key_down(cur, Key::W)) {
    my += 1.f;
  }
  if (key_down(cur, Key::S)) {
    my -= 1.f;
  }
  const float len = std::sqrt(mx * mx + my * my);
  if (len > 1e-5f) {
    a.move_x = mx / len;
    a.move_y = my / len;
  }
  const bool move_nonzero = len > 1e-5f;
  // Win32 視窗將 Escape 用於關閉視窗；暫停用 P（對齊原型 HUD）。
  a.pause_pressed = edge_down(cur, prev, Key::P);
  a.confirm_pressed = edge_down(cur, prev, Key::Space);
  a.dash_pressed = edge_down(cur, prev, Key::Shift) && move_nonzero;
  a.primary_pressed = edge_down(cur, prev, Key::E);
  a.consume_scrap_pressed = edge_down(cur, prev, Key::C);
  return a;
}

}  // namespace weavebound::game_prototype
