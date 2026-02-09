#include "engine/Input.h"

void Input::beginFrame() {
    quitRequested_ = false;
    keyboardState_ = SDL_GetKeyboardState(nullptr);
}

void Input::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_QUIT) {
        quitRequested_ = true;
    }
}

bool Input::isKeyDown(SDL_Scancode key) const {
    if (!keyboardState_) {
        return false;
    }
    return keyboardState_[key] != 0;
}
