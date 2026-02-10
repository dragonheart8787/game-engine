#pragma once

#include <unordered_map>

#include "engine/core/Platform.h"

namespace engine::input {

enum class Action {
  MoveForward,
  MoveBackward,
  MoveLeft,
  MoveRight,
  LookLeft,
  LookRight,
  LookUp,
  LookDown,
  Jump,
  Dash,
  CastAbility1,
  TriggerStoryA,
  TriggerStoryB,
  ToggleDebug
};

class InputSystem {
public:
  void initialize();
  void beginFrame();
  void handleEvent(const engine::core::PlatformEvent& event);

  bool isPressed(Action action) const;

private:
  std::unordered_map<Action, engine::core::KeyCode> bindings_;
  std::unordered_map<engine::core::KeyCode, bool> keyDown_;
};

}  // namespace engine::input
