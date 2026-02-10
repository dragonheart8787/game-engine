#include "engine/math/Random.h"

namespace engine::math {

Random::Random(std::uint64_t seed) : state_(seed ? seed : 0xdeadbeefULL) {}

std::uint32_t Random::nextU32() {
  state_ ^= state_ << 13;
  state_ ^= state_ >> 7;
  state_ ^= state_ << 17;
  return static_cast<std::uint32_t>(state_ & 0xffffffffu);
}

float Random::nextFloat01() {
  return static_cast<float>(nextU32()) / static_cast<float>(0xffffffffu);
}

}  // namespace engine::math
