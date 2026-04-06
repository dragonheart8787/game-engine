#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace weavebound::rg {

enum class PassKind : std::uint8_t { Graphics, Compute, Copy, Resolve };

/** 影像資源在 RG 中的角色（影響 barrier layout / aspect）。 */
enum class ResourceSurfaceKind : std::uint8_t { Color, Depth };

using ResourceId = std::uint32_t;
using PassId = std::uint32_t;

struct ResourceNode {
  ResourceId id{};
  std::string name;
  bool transient{false};
  ResourceSurfaceKind surface{ResourceSurfaceKind::Color};
};

struct PassNode {
  PassId id{};
  std::string name;
  PassKind kind{PassKind::Graphics};
  std::vector<ResourceId> reads;
  std::vector<ResourceId> writes;
};

}  // namespace weavebound::rg
