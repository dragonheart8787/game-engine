#include "engine/math/DeterministicRng.h"

namespace engine::math {

DeterministicRng::DeterministicRng(std::uint64_t seed) {
  setSeed(seed);
}

void DeterministicRng::setSeed(std::uint64_t seed) {
  seed_ = seed == 0 ? 0x1234abcdULL : seed;
  streamStates_.clear();
}

std::uint64_t DeterministicRng::mixState(std::uint64_t x) const {
  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;
  return x;
}

std::uint32_t DeterministicRng::nextU32(std::uint32_t streamId) {
  std::uint64_t& state = streamStates_[streamId];
  if (state == 0) {
    state = mixState(seed_ ^ (static_cast<std::uint64_t>(streamId) << 1U));
  }
  state = mixState(state + 0x9e3779b97f4a7c15ULL);
  return static_cast<std::uint32_t>(state & 0xffffffffu);
}

float DeterministicRng::nextFloat01(std::uint32_t streamId) {
  return static_cast<float>(nextU32(streamId)) / static_cast<float>(0xffffffffu);
}

}  // namespace engine::math
