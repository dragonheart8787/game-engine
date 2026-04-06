// Metal 後端可編譯路徑占位（實作對齊 IDevice 子集後替換）。
#include "weavebound/rhi/device.hpp"

namespace weavebound::rhi {

std::unique_ptr<IDevice> try_create_metal_device(const DeviceDesc&) {
  return nullptr;
}

}  // namespace weavebound::rhi
