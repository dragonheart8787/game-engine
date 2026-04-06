#pragma once

#include <cstdint>

namespace weavebound::rhi {

enum class Backend : std::uint8_t { Vulkan, Metal, D3d12, Count };

enum class PixelFormat : std::uint8_t {
  Undefined,
  R8G8B8A8_UNORM,
  B8G8R8A8_UNORM,
  D32_FLOAT,
};

enum class ResourceState : std::uint8_t {
  Undefined,
  Common,
  RenderTarget,
  Present,
  ShaderResource,
  /** Compute / storage 資源（UAV／SSBO 等） */
  Storage,
  CopyDest,
  CopySrc,
};

enum class QueueKind : std::uint8_t { Graphics, Compute, Copy, Count };

}  // namespace weavebound::rhi
