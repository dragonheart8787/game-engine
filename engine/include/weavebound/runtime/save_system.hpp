#pragma once

#include <string_view>

namespace weavebound::runtime {

/** Save slot + versioning（規格 6）。 */
class ISaveSystem {
public:
  virtual ~ISaveSystem() = default;
  virtual bool write_slot(std::string_view slot_id) = 0;
  virtual bool read_slot(std::string_view slot_id) = 0;
};

}  // namespace weavebound::runtime
