#pragma once

#include <memory>

#include "weavebound/base/macros.hpp"
#include "weavebound/platform/input.hpp"

namespace weavebound::platform {

struct WindowDesc {
  int width_px = 1280;
  int height_px = 720;
  const char* title = "WeaveBound";
  bool visible = true;
};

/**
 * 渲染用視窗 / surface 抽象。上層僅依賴此介面；平台實作放在
 * platform/detail/*（M0 僅提供 stub）。
 */
class IWindow : public NonCopyable {
 public:
  virtual ~IWindow() = default;
  virtual bool is_open() const = 0;
  virtual void set_visible(bool visible) = 0;
  virtual int width() const = 0;
  virtual int height() const = 0;
  /** 原生控制代號；RHI 建立 swapchain 時使用（opaque）。 */
  virtual void* native_display_handle() const = 0;
  virtual void* native_window_handle() const = 0;
  /** 處理一輪視窗訊息（Win32：PeekMessage）；stub 可為空。 */
  virtual void pump_events() {}

  /** 讀取自上次呼叫以來累積的輸入；Win32 實作會將 mouse_dx/dy 清零。 */
  virtual void read_input(InputState& out) {
    out = InputState{};
  }
};

/** M0：無真實視窗，供 CI 與 headless 管線鏈結。 */
std::unique_ptr<IWindow> create_stub_window(const WindowDesc& desc);

#if defined(_WIN32)
/** Win32 真實視窗；`native_display_handle`=HINSTANCE、`native_window_handle`=HWND。 */
std::unique_ptr<IWindow> create_win32_window(const WindowDesc& desc);
#endif

}  // namespace weavebound::platform
