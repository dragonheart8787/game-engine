#include "engine/input/InputSystem.h"

namespace engine::input {

void InputSystem::initialize() {
  bindings_[Action::MoveForward] = engine::core::KeyCode::W;
  bindings_[Action::MoveBackward] = engine::core::KeyCode::S;
  bindings_[Action::MoveLeft] = engine::core::KeyCode::A;
  bindings_[Action::MoveRight] = engine::core::KeyCode::D;
  bindings_[Action::LookLeft] = engine::core::KeyCode::Left;
  bindings_[Action::LookRight] = engine::core::KeyCode::Right;
  bindings_[Action::LookUp] = engine::core::KeyCode::Up;
  bindings_[Action::LookDown] = engine::core::KeyCode::Down;
  bindings_[Action::Jump] = engine::core::KeyCode::Space;
  bindings_[Action::Dash] = engine::core::KeyCode::Shift;
  bindings_[Action::CastAbility1] = engine::core::KeyCode::J;
  bindings_[Action::TriggerStoryA] = engine::core::KeyCode::T;
  bindings_[Action::TriggerStoryB] = engine::core::KeyCode::Y;
  bindings_[Action::ToggleDebug] = engine::core::KeyCode::L;
}

void InputSystem::beginFrame() {
}

void InputSystem::handleEvent(const engine::core::PlatformEvent& event) {
  if (event.type == engine::core::PlatformEventType::KeyDown) {
    keyDown_[event.key] = true;
  } else if (event.type == engine::core::PlatformEventType::KeyUp) {
    keyDown_[event.key] = false;
  }
}

bool InputSystem::isPressed(Action action) const {
  const auto it = bindings_.find(action);
  if (it == bindings_.end()) {
    return false;
  }
  const auto keyIt = keyDown_.find(it->second);
  if (keyIt == keyDown_.end()) {
    return false;
  }
  return keyIt->second;
}

}  // namespace engine::input
