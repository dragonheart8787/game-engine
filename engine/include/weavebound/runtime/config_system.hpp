#pragma once

#include <string_view>

namespace weavebound::runtime {

/** Config：ini/json（規格 6）。 */
class IConfigSystem {
public:
  virtual ~IConfigSystem() = default;
  virtual bool load_file(std::string_view path) = 0;
};

}  // namespace weavebound::runtime
