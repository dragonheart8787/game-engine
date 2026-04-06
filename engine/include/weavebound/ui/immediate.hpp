#pragma once

namespace weavebound::ui {

/** ImGui 類 immediate UI（規格 1.9 debug/開發）。 */
class IImmediateUi {
public:
  virtual ~IImmediateUi() = default;
  virtual void new_frame() = 0;
  virtual void render() = 0;
};

}  // namespace weavebound::ui
