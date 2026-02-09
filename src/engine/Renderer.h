#pragma once

#include <SDL.h>

class Renderer {
public:
    bool initialize(SDL_Window* window);
    void beginFrame();
    void endFrame();
    void shutdown();

private:
    SDL_Renderer* renderer_ = nullptr;
};
