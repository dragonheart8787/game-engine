#pragma once

#include <cstddef>
#include <cstdint>

#include "weavebound/rhi/types.hpp"

namespace weavebound::rhi {

/** Buffer / Image / Sampler 控制代碼占位（規格 1.2 Resource）。 */
struct BufferHandle { std::uint64_t id{}; };
struct ImageHandle { std::uint64_t id{}; };
struct SamplerHandle { std::uint64_t id{}; };

enum class BufferUsage : std::uint32_t {
  None = 0,
  Vertex = 1u << 0,
  Index = 1u << 1,
  Uniform = 1u << 2,
  TransferSrc = 1u << 3,
  TransferDst = 1u << 4,
  Storage = 1u << 5,
};

constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) {
  return static_cast<BufferUsage>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr std::uint32_t as_u32(BufferUsage u) { return static_cast<std::uint32_t>(u); }

struct BufferDesc {
  std::size_t size_bytes{};
  /** HOST_VISIBLE|HOST_COHERENT for CPU map / staging；否則 DEVICE_LOCAL。 */
  bool host_visible{false};
  BufferUsage usage{BufferUsage::None};
};

enum class ImageUsageFlags : std::uint32_t {
  None = 0,
  Sampled = 1u << 0,
  ColorAttachment = 1u << 1,
  DepthStencil = 1u << 2,
  TransferDst = 1u << 3,
  TransferSrc = 1u << 4,
};

constexpr ImageUsageFlags operator|(ImageUsageFlags a, ImageUsageFlags b) {
  return static_cast<ImageUsageFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

constexpr std::uint32_t as_u32(ImageUsageFlags u) { return static_cast<std::uint32_t>(u); }

struct ImageDesc {
  std::uint32_t width_px{1};
  std::uint32_t height_px{1};
  PixelFormat format{PixelFormat::R8G8B8A8_UNORM};
  ImageUsageFlags usage{ImageUsageFlags::None};
};

}  // namespace weavebound::rhi
