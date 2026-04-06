#pragma once

#include <cstddef>
#include <cstdint>

namespace weavebound::rhi {

/** Graphics / Compute PSO 描述占位（規格 1.2；後端實作對齊 SPIR-V / MSLL / DXIL）。 */
struct GraphicsPsoDesc {
  const void* vertex_shader_bytecode{};
  std::size_t vertex_shader_size{};
  const void* fragment_shader_bytecode{};
  std::size_t fragment_shader_size{};
};

struct ComputePsoDesc {
  const void* shader_bytecode{};
  std::size_t shader_size{};
};

struct PsoHandle {
  std::uint64_t id{};
};

}  // namespace weavebound::rhi
