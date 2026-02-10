#pragma once

#include <cstdint>
#include <unordered_map>

namespace engine::math {

class DeterministicRng {
public:
  explicit DeterministicRng(std::uint64_t seed = 0);

  void setSeed(std::uint64_t seed);
  std::uint32_t nextU32(std::uint32_t streamId);
  float nextFloat01(std::uint32_t streamId);

private:
  std::uint64_t mixState(std::uint64_t x) const;

  std::uint64_t seed_ = 0;
  std::unordered_map<std::uint32_t, std::uint64_t> streamStates_;
};

}  // namespace engine::math
