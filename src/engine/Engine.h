#pragma once

#include <SDL.h>
#include <cstdint>

#include "engine/Renderer.h"

class Engine {
public:
    bool initialize();
    void run();
    void shutdown();

private:
    void processInput();
    void update(float deltaSeconds);
    void render();

    bool isRunning_ = false;
    SDL_Window* window_ = nullptr;
    Renderer renderer_;
    std::uint64_t lastCounter_ = 0;
};
