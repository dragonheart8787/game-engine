#pragma once

#include <SDL.h>

struct Transform;

class Renderer {
public:
    bool initialize(SDL_Window* window);
    void beginFrame();
    void drawRect(const Transform& transform, SDL_Color color);
    void endFrame();
    void shutdown();

private:
    SDL_Renderer* renderer_ = nullptr;
};
