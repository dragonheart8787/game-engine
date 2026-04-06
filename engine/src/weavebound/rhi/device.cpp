#include "weavebound/rhi/device.hpp"

#if defined(WB_HAS_VULKAN) && WB_HAS_VULKAN
namespace weavebound::rhi {
std::unique_ptr<IDevice> try_create_vulkan_device(const DeviceDesc& desc);
}
#endif

namespace weavebound::rhi {

IDevice::~IDevice() = default;

std::unique_ptr<IDevice> create_device(const DeviceDesc& desc) {
#if defined(WB_HAS_VULKAN) && WB_HAS_VULKAN
  if (desc.backend == Backend::Vulkan) {
    auto v = try_create_vulkan_device(desc);
    if (v && v->is_valid()) {
      return v;
    }
  }
#endif
  return nullptr;
}

}  // namespace weavebound::rhi
