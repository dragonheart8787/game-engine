#pragma once

#include <SDL.h>

class Input {
public:
    void beginFrame();
    void handleEvent(const SDL_Event& event);

    bool quitRequested() const { return quitRequested_; }
    bool isKeyDown(SDL_Scancode key) const;

private:
    const std::uint8_t* keyboardState_ = nullptr;
    bool quitRequested_ = false;
};
