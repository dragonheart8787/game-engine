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

void InputSystem::beginFrame() {}

void InputSystem::handleEvent(const engine::core::PlatformEvent& event) {
  if (event.type == engine::core::PlatformEventType::KeyDown) {
    keyDown_[event.key] = true;
  } else if (event.type == engine::core::PlatformEventType::KeyUp) {
    keyDown_[event.key] = false;
  } else if (event.type == engine::core::PlatformEventType::TouchDown ||
             event.type == engine::core::PlatformEventType::TouchMove) {
    // Android stub mapping: left half joystick for move, right half for look/cast.
    if (event.touchX < 0.5f) {
      keyDown_[engine::core::KeyCode::W] = event.touchY < 0.4f;
      keyDown_[engine::core::KeyCode::S] = event.touchY > 0.6f;
      keyDown_[engine::core::KeyCode::A] = event.touchX < 0.25f;
      keyDown_[engine::core::KeyCode::D] = event.touchX > 0.25f;
    } else {
      keyDown_[engine::core::KeyCode::J] = (event.touchX > 0.8f && event.touchY > 0.7f);
      keyDown_[engine::core::KeyCode::Space] = (event.touchX > 0.8f && event.touchY < 0.3f);
      keyDown_[engine::core::KeyCode::Shift] = (event.touchX > 0.6f && event.touchX < 0.8f);
    }
  } else if (event.type == engine::core::PlatformEventType::TouchUp) {
    keyDown_[engine::core::KeyCode::W] = false;
    keyDown_[engine::core::KeyCode::A] = false;
    keyDown_[engine::core::KeyCode::S] = false;
    keyDown_[engine::core::KeyCode::D] = false;
    keyDown_[engine::core::KeyCode::J] = false;
    keyDown_[engine::core::KeyCode::Space] = false;
    keyDown_[engine::core::KeyCode::Shift] = false;
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
