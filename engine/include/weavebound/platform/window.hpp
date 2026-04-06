#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "weavebound/base/macros.hpp"
#include "weavebound/platform/input.hpp"

namespace weavebound::platform {

/** `--play` 頂視簡圖資料（Win32 以 GDI 繪製；`active==false` 則不畫）。 */
struct ClientMinimapFrame {
  bool active{false};
  float world_min_x{0.f};
  float world_min_y{0.f};
  float world_max_x{1.f};
  float world_max_y{1.f};
  float player_x{0.f};
  float player_y{0.f};
  float player_yaw_rad{0.f};
  float goal_x{0.f};
  float goal_y{0.f};
  float goal_radius{1.f};
  static constexpr int kMaxBlips = 64;
  int blip_count{0};
  float blip_x[kMaxBlips]{};
  float blip_y[kMaxBlips]{};
  /** Win32 `COLORREF`（0x00bbggrr），例如 `0x000000FF` 為紅。 */
  std::uint32_t blip_rgb[kMaxBlips]{};
};

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

  /** 要求關閉視窗（Win32：Post WM_CLOSE；stub：標記關閉以便 tick 結束）。 */
  virtual void request_close() {}

  /** Win32：`--play` 無 Vulkan 時在客戶區以 GDI 顯示 UTF-8 多行文字；其他平台預設忽略。 */
  virtual void set_client_overlay_utf8(const std::string& utf8_text) {
    (void)utf8_text;
  }

  /** 傳 `nullptr` 或 `active==false` 關閉頂視簡圖。 */
  virtual void set_client_minimap(const ClientMinimapFrame* frame) {
    (void)frame;
  }
};

/** M0：無真實視窗，供 CI 與 headless 管線鏈結。 */
std::unique_ptr<IWindow> create_stub_window(const WindowDesc& desc);

#if defined(_WIN32)
/** Win32 真實視窗；`native_display_handle`=HINSTANCE、`native_window_handle`=HWND。 */
std::unique_ptr<IWindow> create_win32_window(const WindowDesc& desc);
#endif

}  // namespace weavebound::platform
