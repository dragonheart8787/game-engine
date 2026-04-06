#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace weavebound::platform {

/** CPU/GPU/記憶體/語系（規格 1.1 System）。 */
struct SystemInfoSnapshot {
  std::uint32_t logical_cpu_cores{};
  std::uint64_t total_ram_bytes{};
  std::string   gpu_description;
  std::string   locale_tag;
};

class ISystemInfo {
public:
  virtual ~ISystemInfo() = default;
  virtual SystemInfoSnapshot query() const = 0;
};

}  // namespace weavebound::platform
