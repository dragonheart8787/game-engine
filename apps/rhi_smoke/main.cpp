// CTest：Vulkan 存在時建立 1x1 影像並釋放（不依賴視窗）。
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

#include "weavebound/engine/application.hpp"
#include "weavebound/rhi/device.hpp"
#include "weavebound/rhi/resources.hpp"

int main() {
#if !defined(_WIN32)
  return 0;
#else
  weavebound::engine::ApplicationConfig cfg;
  cfg.request_vulkan = true;
  cfg.visible = false;
  weavebound::engine::Application app;
  if (!app.startup(cfg)) {
    return 0;
  }
  weavebound::rhi::IDevice* dev = app.device();
  if (!dev || !dev->is_valid()) {
    return 0;
  }
  weavebound::rhi::ImageDesc id{};
  id.width_px = 1;
  id.height_px = 1;
  id.format = weavebound::rhi::PixelFormat::R8G8B8A8_UNORM;
  id.usage = weavebound::rhi::ImageUsageFlags::Sampled | weavebound::rhi::ImageUsageFlags::TransferDst;
  weavebound::rhi::ImageHandle ih{};
  if (!dev->create_image(id, ih)) {
    return 1;
  }
  dev->destroy_image(ih);
  return 0;
#endif
}
