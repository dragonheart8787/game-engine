#include "weavebound/platform/window.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#include <imgui.h>

// ImGui 1.87+：imgui_impl_win32.h 以 #if 0 包住 WndProc 宣告，避免拖入 <windows.h>；
// 此 TU 已含 Windows 型別，需自行前向宣告（與 imgui_impl_win32.cpp 定義連結）。
IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
      if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
        open_ = false;
        PostQuitMessage(0);
        break;
      }
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  void read_input(InputState& out) override {
    out = pending_;
    pending_.mouse_dx = 0;
    pending_.mouse_dy = 0;
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
};

}  // namespace

std::unique_ptr<IWindow> create_win32_window(const WindowDesc& desc) { return std::make_unique<WindowWin32>(desc); }

}  // namespace weavebound::platform
