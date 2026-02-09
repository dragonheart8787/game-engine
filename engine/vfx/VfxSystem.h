#pragma once

#include <string>

namespace engine::vfx {

class VfxSystem {
public:
  void emit(const std::string& eventType) { lastEvent_ = eventType; }
  const std::string& lastEvent() const { return lastEvent_; }

private:
  std::string lastEvent_;
};

}  // namespace engine::vfx
