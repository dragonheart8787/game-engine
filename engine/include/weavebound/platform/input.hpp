#pragma once

#include <cstdint>

namespace weavebound::platform {

enum class Key : std::uint32_t {
  Unknown = 0,
  Escape,
  W,
  A,
  S,
  D,
  Space,
  P,
  N,
  L,
  K,
  M,
  V,
  Shift,
  E,
  C,
  Count
};

enum class MouseButton : std::uint8_t { Left, Right, Middle };

enum class Modifier : std::uint8_t { None = 0, Shift = 1 << 0, Ctrl = 1 << 1, Alt = 1 << 2 };

/** 單幀輸入快照（MVP）：鍵為 held bitmask，滑鼠增量在 read_input 後清零。 */
struct InputState {
  std::uint32_t keys_held{0};
  std::uint8_t mouse_buttons{0};
  std::int32_t mouse_x{0};
  std::int32_t mouse_y{0};
  std::int32_t mouse_dx{0};
  std::int32_t mouse_dy{0};
};

constexpr std::uint8_t kInputMouseLeft = 1u << 0;
constexpr std::uint8_t kInputMouseRight = 1u << 1;
constexpr std::uint8_t kInputMouseMiddle = 1u << 2;

inline bool key_down(InputState s, Key k) {
  if (k == Key::Unknown || k >= Key::Count) {
    return false;
  }
  return (s.keys_held & (1u << static_cast<unsigned>(k))) != 0;
}

inline bool mouse_down(InputState s, MouseButton b) {
  switch (b) {
    case MouseButton::Left:
      return (s.mouse_buttons & kInputMouseLeft) != 0;
    case MouseButton::Right:
      return (s.mouse_buttons & kInputMouseRight) != 0;
    case MouseButton::Middle:
      return (s.mouse_buttons & kInputMouseMiddle) != 0;
  }
  return false;
}

/** 之後與 rebind 表、文字輸入對齊；M0 僅型別占位。 */
struct InputEvent {
  enum class Kind { Key, MouseMove, MouseButton, Text };
  Kind kind{Kind::Key};
};

}  // namespace weavebound::platform
