#pragma once

#include <SDL.h>
#include <unordered_map>

namespace engine::input {

enum class Action {
  MoveForward,
  MoveBackward,
  MoveLeft,
  MoveRight,
  Jump,
  Dash,
  Ability1,
  Ability2
};

class InputSystem {
public:
  void initialize();
  void beginFrame();
  void handleEvent(const SDL_Event& event);

  bool isPressed(Action action) const;

private:
  const std::uint8_t* keyboardState_ = nullptr;
  std::unordered_map<Action, SDL_Scancode> bindings_;
};

}  // namespace engine::input
