#pragma once

#include <cstdint>
#include <string>

namespace engine::debug {

struct DebugSnapshot {
  std::uint64_t fixedTick = 0;
  std::uint64_t worldHash = 0;
  std::uint32_t rngStreamId = 0;
  std::size_t deltaCount = 0;
  std::string storyBeat;
  std::string controlMask;
  float abilityCooldown = 0.0f;
};

class DebugOverlay {
public:
  void initialize() {}
  void beginFrame() {}
  void setSnapshot(const DebugSnapshot& snapshot) { snapshot_ = snapshot; }
  const DebugSnapshot& snapshot() const { return snapshot_; }
  void endFrame() {}

private:
  DebugSnapshot snapshot_{};
};

}  // namespace engine::debug
