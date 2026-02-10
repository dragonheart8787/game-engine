#pragma once

#include <cstdint>

namespace engine::math {

class Random {
public:
  explicit Random(std::uint64_t seed = 0);

  std::uint32_t nextU32();
  float nextFloat01();

private:
  std::uint64_t state_ = 0;
};

}  // namespace engine::math
