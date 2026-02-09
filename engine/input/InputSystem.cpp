#include "engine/input/InputSystem.h"

namespace engine::input {

void InputSystem::initialize() {
  bindings_[Action::MoveForward] = SDL_SCANCODE_W;
  bindings_[Action::MoveBackward] = SDL_SCANCODE_S;
  bindings_[Action::MoveLeft] = SDL_SCANCODE_A;
  bindings_[Action::MoveRight] = SDL_SCANCODE_D;
  bindings_[Action::Jump] = SDL_SCANCODE_SPACE;
  bindings_[Action::Dash] = SDL_SCANCODE_LSHIFT;
  bindings_[Action::Ability1] = SDL_SCANCODE_J;
  bindings_[Action::Ability2] = SDL_SCANCODE_K;
}

void InputSystem::beginFrame() {
  keyboardState_ = SDL_GetKeyboardState(nullptr);
}

void InputSystem::handleEvent(const SDL_Event& /*event*/) {}

bool InputSystem::isPressed(Action action) const {
  const auto it = bindings_.find(action);
  if (it == bindings_.end() || !keyboardState_) {
    return false;
  }
  return keyboardState_[it->second] != 0;
}

}  // namespace engine::input
