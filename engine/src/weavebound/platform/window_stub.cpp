#include "weavebound/platform/window.hpp"

#include <memory>
#include <utility>

namespace weavebound::platform {

namespace {

class WindowStub final : public IWindow {
 public:
  explicit WindowStub(WindowDesc desc) : desc_(std::move(desc)) {}

  bool is_open() const override { return open_; }

  void set_visible(bool visible) override { desc_.visible = visible; }

  int width() const override { return desc_.width_px; }

  int height() const override { return desc_.height_px; }

  void* native_display_handle() const override { return nullptr; }

  void* native_window_handle() const override { return nullptr; }

  void pump_events() override {}

  void read_input(InputState& out) override { out = InputState{}; }

 private:
  WindowDesc desc_;
  bool open_{true};
};

}  // namespace

std::unique_ptr<IWindow> create_stub_window(const WindowDesc& desc) {
  return std::make_unique<WindowStub>(desc);
}

}  // namespace weavebound::platform
