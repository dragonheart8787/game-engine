#include "weavebound/platform/input.hpp"
#include "weavebound/platform/window.hpp"

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

int main() {
#if defined(_WIN32)
  using namespace weavebound::platform;

  WindowDesc wd{};
  wd.width_px = 320;
  wd.height_px = 240;
  wd.title = "input_smoke";
  wd.visible = false;

  auto wnd = create_win32_window(wd);
  if (!wnd || !wnd->is_open()) {
    return 1;
  }

  HWND hwnd = static_cast<HWND>(wnd->native_window_handle());
  SendMessageW(hwnd, WM_KEYDOWN, 'W', 0);
  wnd->pump_events();

  InputState st{};
  wnd->read_input(st);
  if (!key_down(st, Key::W)) {
    return 2;
  }

  SendMessageW(hwnd, WM_KEYUP, 'W', 0);
  wnd->pump_events();
  wnd->read_input(st);
  if (key_down(st, Key::W)) {
    return 3;
  }

  return 0;
#else
  return 0;
#endif
}
