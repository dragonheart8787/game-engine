#include "weavebound/platform/window.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#include <imgui.h>

// ImGui 1.87+：imgui_impl_win32.h 以 #if 0 包住 WndProc 宣告，避免拖入 <windows.h>；
// 此 TU 已含 Windows 型別，需自行前向宣告（與 imgui_impl_win32.cpp 定義連結）。
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <memory>
#include <string>
#include <vector>

namespace weavebound::platform {

namespace {

Key vk_to_key(WPARAM vk) {
  switch (vk) {
    case VK_ESCAPE:
      return Key::Escape;
    case 'W':
    case 'w':
      return Key::W;
    case 'A':
    case 'a':
      return Key::A;
    case 'S':
    case 's':
      return Key::S;
    case 'D':
    case 'd':
      return Key::D;
    case VK_SPACE:
      return Key::Space;
    case 'P':
    case 'p':
      return Key::P;
    case 'N':
    case 'n':
      return Key::N;
    case 'L':
    case 'l':
      return Key::L;
    case 'K':
    case 'k':
      return Key::K;
    case 'M':
    case 'm':
      return Key::M;
    case 'V':
    case 'v':
      return Key::V;
    case 'Q':
    case 'q':
      return Key::Q;
    case 'E':
    case 'e':
      return Key::E;
    case 'C':
    case 'c':
      return Key::C;
    case VK_SHIFT:
      return Key::Shift;
    default:
      return Key::Unknown;
  }
}

static std::wstring utf8_to_wide(const char* utf8) {
  if (!utf8) {
    return L"WeaveBound";
  }
  const int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
  if (n <= 0) {
    return L"WeaveBound";
  }
  std::vector<wchar_t> buf(static_cast<size_t>(n));
  if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buf.data(), n) <= 0) {
    return L"WeaveBound";
  }
  return std::wstring(buf.data());
}

static std::wstring utf8_to_wide_allow_empty(const std::string& utf8) {
  if (utf8.empty()) {
    return {};
  }
  const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
  if (n <= 0) {
    return {};
  }
  std::wstring w(static_cast<size_t>(n), L'\0');
  if (MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), &w[0], n) <= 0) {
    return {};
  }
  return w;
}

static bool world_to_minimap_px(float wx, float wy, const ClientMinimapFrame& f, const RECT& mapInner, int* out_x,
                                int* out_y, float* out_scale) {
  const float fw = f.world_max_x - f.world_min_x;
  const float fh = f.world_max_y - f.world_min_y;
  if (fw < 1e-5f || fh < 1e-5f) {
    return false;
  }
  const int mw = mapInner.right - mapInner.left;
  const int mh = mapInner.bottom - mapInner.top;
  if (mw < 8 || mh < 8) {
    return false;
  }
  const float scale = (std::min)(static_cast<float>(mw) / fw, static_cast<float>(mh) / fh);
  const float cx = (f.world_min_x + f.world_max_x) * 0.5f;
  const float cy = (f.world_min_y + f.world_max_y) * 0.5f;
  const float nx = (wx - cx) * scale;
  const float ny = (wy - cy) * scale;
  const int midx = (mapInner.left + mapInner.right) / 2;
  const int midy = (mapInner.top + mapInner.bottom) / 2;
  *out_x = midx + static_cast<int>(nx);
  *out_y = midy + static_cast<int>(ny);
  if (out_scale) {
    *out_scale = scale;
  }
  return true;
}

static void draw_minimap_gdi(HDC hdc, const RECT& mapInner, const ClientMinimapFrame& f) {
  HPEN grid_pen = CreatePen(PS_SOLID, 1, RGB(26, 32, 44));
  HGDIOBJ old_pen0 = SelectObject(hdc, grid_pen);
  for (int x = mapInner.left; x < mapInner.right; x += 40) {
    MoveToEx(hdc, x, mapInner.top, nullptr);
    LineTo(hdc, x, mapInner.bottom);
  }
  for (int y = mapInner.top; y < mapInner.bottom; y += 40) {
    MoveToEx(hdc, mapInner.left, y, nullptr);
    LineTo(hdc, mapInner.right, y);
  }
  SelectObject(hdc, old_pen0);
  DeleteObject(grid_pen);

  HFONT label_font = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_DONTCARE,
                                 L"Microsoft JhengHei UI");
  HGDIOBJ old_font = SelectObject(hdc, label_font ? label_font : GetStockObject(DEFAULT_GUI_FONT));
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, RGB(195, 205, 225));
  const wchar_t* const k_title = L"WeaveBound — 戰場（頂視）";
  TextOutW(hdc, mapInner.left + 8, mapInner.top + 6, k_title, static_cast<int>(std::wcslen(k_title)));
  SelectObject(hdc, old_font);
  if (label_font) {
    DeleteObject(label_font);
  }

  HPEN null_pen = static_cast<HPEN>(GetStockObject(NULL_PEN));
  HGDIOBJ old_pen = SelectObject(hdc, null_pen);
  HBRUSH stock_br = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

  int gx = 0;
  int gy = 0;
  float scale = 1.f;
  const bool have_goal = world_to_minimap_px(f.goal_x, f.goal_y, f, mapInner, &gx, &gy, &scale);
  if (have_goal) {
    const int gr = (std::max)(4, static_cast<int>(f.goal_radius * scale + 0.5f));
    HBRUSH br_g = CreateSolidBrush(RGB(40, 150, 75));
    HGDIOBJ old_br = SelectObject(hdc, br_g);
    Ellipse(hdc, gx - gr, gy - gr, gx + gr, gy + gr);
    SelectObject(hdc, old_br);
    DeleteObject(br_g);
  }

  for (int i = 0; i < f.blip_count; ++i) {
    int bx = 0;
    int by = 0;
    float sc = 1.f;
    if (!world_to_minimap_px(f.blip_x[i], f.blip_y[i], f, mapInner, &bx, &by, &sc)) {
      continue;
    }
    const int r = 4;
    HBRUSH b = CreateSolidBrush(static_cast<COLORREF>(f.blip_rgb[i]));
    SelectObject(hdc, b);
    Ellipse(hdc, bx - r, by - r, bx + r, by + r);
    SelectObject(hdc, stock_br);
    DeleteObject(b);
  }

  int px = 0;
  int py = 0;
  if (world_to_minimap_px(f.player_x, f.player_y, f, mapInner, &px, &py, &scale)) {
    const int pr = 7;
    HBRUSH pb = CreateSolidBrush(RGB(110, 220, 255));
    SelectObject(hdc, pb);
    Ellipse(hdc, px - pr, py - pr, px + pr, py + pr);
    SelectObject(hdc, stock_br);
    DeleteObject(pb);

    const int line_len = (std::max)(12, static_cast<int>(20.f * scale));
    const int lx = px + static_cast<int>(std::sin(static_cast<double>(f.player_yaw_rad)) * line_len);
    const int ly = py + static_cast<int>(std::cos(static_cast<double>(f.player_yaw_rad)) * line_len);
    HPEN line_pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
    HGDIOBJ old_lp = SelectObject(hdc, line_pen);
    MoveToEx(hdc, px, py, nullptr);
    LineTo(hdc, lx, ly);
    SelectObject(hdc, old_lp);
    DeleteObject(line_pen);
  }

  SelectObject(hdc, old_pen);

  HFONT leg_font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                               CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_DONTCARE,
                               L"Microsoft JhengHei UI");
  HGDIOBJ old_f2 = SelectObject(hdc, leg_font ? leg_font : GetStockObject(DEFAULT_GUI_FONT));
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, RGB(160, 175, 200));
  const wchar_t* const k_leg = L"圖例：綠＝終點 · 淺藍＝你 · 紅／橘＝敵";
  TextOutW(hdc, mapInner.left + 8, mapInner.bottom - 22, k_leg, static_cast<int>(std::wcslen(k_leg)));
  SelectObject(hdc, old_f2);
  if (leg_font) {
    DeleteObject(leg_font);
  }
}

class WindowWin32 final : public IWindow {
 public:
  explicit WindowWin32(const WindowDesc& desc)
      : w_(desc.width_px), h_(desc.height_px), hinst_(GetModuleHandleW(nullptr)) {
    static const wchar_t kCls[] = L"WeaveBoundPlatformWnd_v1";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hinst_;
    wc.hCursor = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
    // 未設定時 hbrBackground 為 NULL，系統要求應用處理 WM_PAINT；否則易造成客戶區無效區域堆積、視窗顯示「沒有回應」。
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = kCls;
    const ATOM atom = RegisterClassExW(&wc);
    if (!atom) {
      const DWORD err = GetLastError();
      if (err != ERROR_CLASS_ALREADY_EXISTS) {
        open_ = false;
        return;
      }
    }

    const std::wstring title = utf8_to_wide(desc.title);
    const DWORD style = WS_OVERLAPPEDWINDOW;
    RECT r{0, 0, w_, h_};
    AdjustWindowRect(&r, style, FALSE);
    const int ww = r.right - r.left;
    const int wh = r.bottom - r.top;
    hwnd_ = CreateWindowExW(0, kCls, title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT, ww, wh, nullptr,
                            nullptr, hinst_, this);
    if (!hwnd_) {
      open_ = false;
      return;
    }
    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(hwnd_, desc.visible ? SW_SHOW : SW_HIDE);
    UpdateWindow(hwnd_);
  }

  ~WindowWin32() override {
    if (overlay_font_) {
      DeleteObject(overlay_font_);
      overlay_font_ = nullptr;
    }
    if (hwnd_) {
      SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
      DestroyWindow(hwnd_);
    }
    hwnd_ = nullptr;
  }

  bool is_open() const override { return open_ && hwnd_ != nullptr; }

  void set_visible(bool visible) override {
    if (!hwnd_) {
      return;
    }
    ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
  }

  int width() const override { return w_; }

  int height() const override { return h_; }

  void* native_display_handle() const override { return static_cast<void*>(hinst_); }

  void* native_window_handle() const override { return static_cast<void*>(hwnd_); }

  void pump_events() override {
    if (!hwnd_) {
      return;
    }
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        open_ = false;
        break;
      }
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  void request_close() override {
    if (hwnd_) {
      PostMessageW(hwnd_, WM_CLOSE, 0, 0);
    }
  }

  void read_input(InputState& out) override {
    out = pending_;
    pending_.mouse_dx = 0;
    pending_.mouse_dy = 0;
  }

  void set_client_overlay_utf8(const std::string& utf8_text) override {
    if (!hwnd_) {
      return;
    }
    overlay_w_ = utf8_to_wide_allow_empty(utf8_text);
    InvalidateRect(hwnd_, nullptr, FALSE);
  }

  void set_client_minimap(const ClientMinimapFrame* frame) override {
    if (frame && frame->active) {
      minimap_ = *frame;
      minimap_on_ = true;
    } else {
      minimap_on_ = false;
    }
    if (hwnd_) {
      InvalidateRect(hwnd_, nullptr, FALSE);
    }
  }

  void mark_closed() { open_ = false; }

  void apply_key(WPARAM vk, bool down) {
    const Key k = vk_to_key(vk);
    if (k == Key::Unknown) {
      return;
    }
    const std::uint32_t bit = 1u << static_cast<unsigned>(k);
    if (down) {
      pending_.keys_held |= bit;
    } else {
      pending_.keys_held &= ~bit;
    }
  }

  void on_mouse_move(LPARAM lParam) {
    const int x = static_cast<int>(GET_X_LPARAM(lParam));
    const int y = static_cast<int>(GET_Y_LPARAM(lParam));
    if (has_mouse_) {
      pending_.mouse_dx += x - last_mx_;
      pending_.mouse_dy += y - last_my_;
    } else {
      has_mouse_ = true;
    }
    last_mx_ = x;
    last_my_ = y;
    pending_.mouse_x = x;
    pending_.mouse_y = y;
  }

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui::GetCurrentContext() != nullptr) {
      const LRESULT ir = ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
      if (ir != 0) {
        return ir;
      }
    }
    WindowWin32* self = reinterpret_cast<WindowWin32*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_DESTROY) {
      if (self) {
        self->mark_closed();
      }
      PostQuitMessage(0);
      return 0;
    }
    if (msg == WM_PAINT) {
      WindowWin32* paint_self = reinterpret_cast<WindowWin32*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
      PAINTSTRUCT ps{};
      const HDC hdc = BeginPaint(hwnd, &ps);
      if (hdc) {
        RECT rc{};
        GetClientRect(hwnd, &rc);
        RECT text_rc = rc;
        const bool split = paint_self && paint_self->minimap_on_ && paint_self->minimap_.active;
        if (split) {
          const int ch = rc.bottom - rc.top;
          const int split_y = rc.top + (ch * 68) / 100;
          RECT map_rc = rc;
          map_rc.bottom = split_y;
          text_rc.top = split_y;
          HBRUSH map_bg = CreateSolidBrush(RGB(10, 12, 20));
          FillRect(hdc, &map_rc, map_bg);
          DeleteObject(map_bg);
          RECT map_inner = map_rc;
          InflateRect(&map_inner, -8, -8);
          draw_minimap_gdi(hdc, map_inner, paint_self->minimap_);
          HPEN border = CreatePen(PS_SOLID, 1, RGB(50, 55, 70));
          HGDIOBJ op = SelectObject(hdc, border);
          HGDIOBJ obr = SelectObject(hdc, GetStockObject(NULL_BRUSH));
          MoveToEx(hdc, map_rc.left, map_rc.bottom, nullptr);
          LineTo(hdc, map_rc.right, map_rc.bottom);
          SelectObject(hdc, obr);
          SelectObject(hdc, op);
          DeleteObject(border);
          FillRect(hdc, &text_rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        } else {
          FillRect(hdc, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        }
        if (paint_self && !paint_self->overlay_w_.empty()) {
          HFONT fnt = paint_self->overlay_font_;
          if (!fnt) {
            fnt = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_DONTCARE,
                              L"Microsoft JhengHei UI");
            if (fnt) {
              paint_self->overlay_font_ = fnt;
            }
          }
          if (!fnt) {
            fnt = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
          }
          HGDIOBJ old_font = SelectObject(hdc, fnt);
          SetBkMode(hdc, TRANSPARENT);
          SetTextColor(hdc, RGB(235, 235, 235));
          RECT trc = text_rc;
          trc.left += 12;
          trc.top += 10;
          trc.right -= 12;
          trc.bottom -= 8;
          DrawTextW(hdc, paint_self->overlay_w_.c_str(), -1, &trc,
                    DT_LEFT | DT_TOP | DT_WORDBREAK | DT_EXPANDTABS | DT_NOPREFIX);
          SelectObject(hdc, old_font);
        }
      }
      EndPaint(hwnd, &ps);
      return 0;
    }
    if (msg == WM_SIZE && self) {
      const int nw = static_cast<int>(LOWORD(lParam));
      const int nh = static_cast<int>(HIWORD(lParam));
      if (nw > 0 && nh > 0) {
        self->w_ = nw;
        self->h_ = nh;
      }
      return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    if (self) {
      if (msg == WM_KEYDOWN) {
        self->apply_key(wParam, true);
      } else if (msg == WM_KEYUP) {
        self->apply_key(wParam, false);
      } else if (msg == WM_LBUTTONDOWN) {
        self->pending_.mouse_buttons |= kInputMouseLeft;
      } else if (msg == WM_LBUTTONUP) {
        self->pending_.mouse_buttons &= static_cast<std::uint8_t>(~kInputMouseLeft);
      } else if (msg == WM_MOUSEMOVE) {
        self->on_mouse_move(lParam);
      }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }

 private:
  HWND hwnd_{nullptr};
  HINSTANCE hinst_{nullptr};
  bool open_{true};
  int w_{1280};
  int h_{720};
  InputState pending_{};
  bool has_mouse_{false};
  int last_mx_{0};
  int last_my_{0};
  std::wstring overlay_w_{};
  HFONT overlay_font_{nullptr};
  ClientMinimapFrame minimap_{};
  bool minimap_on_{false};
};

}  // namespace

std::unique_ptr<IWindow> create_win32_window(const WindowDesc& desc) { return std::make_unique<WindowWin32>(desc); }

}  // namespace weavebound::platform
