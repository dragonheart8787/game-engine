#pragma once

#include <cstdint>

namespace weavebound::rhi {

/**
 * Descriptor set / root signature / Metal argument buffer 統一占位（規格 1.2 Binding）。
 */
class IDescriptorLayout {
 public:
  virtual ~IDescriptorLayout() = default;
};

struct DescriptorSetHandle {
  std::uint64_t id{};
};

}  // namespace weavebound::rhi
