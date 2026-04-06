#pragma once

#include "weavebound/platform/input.hpp"

namespace weavebound::game_prototype {

/** 遊戲層邏輯動作（對齊 ADR 0004；MVP 僅鍵盤）。 */
struct GameplayActions {
  float move_x{0.f};
  float move_y{0.f};
  bool pause_pressed{false};
  bool confirm_pressed{false};
  /** Shift 邊緣觸發且本幀有移動輸入時為真（衝刺）。 */
  bool dash_pressed{false};
  /** E 鍵邊緣觸發（主要能力）。 */
  bool primary_pressed{false};
  /** C 鍵邊緣觸發（消耗碎片補 Focus）。 */
  bool consume_scrap_pressed{false};
};

/** 由 Raw InputState 與前幀 held 解出邊緣觸發。 */
GameplayActions build_gameplay_actions(const weavebound::platform::InputState& cur,
                                       const weavebound::platform::InputState& prev);

}  // namespace weavebound::game_prototype
