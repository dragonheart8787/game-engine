#pragma once

#include <SDL.h>

#include "engine/Input.h"
#include "engine/Renderer.h"
#include "engine/Time.h"
#include "engine/World.h"

class Engine {
public:
    bool initialize();
    void run();
    void shutdown();

private:
    void processInput();
    void update();
    void render();
    void clampToWindow(Transform& transform);

    bool isRunning_ = false;
    SDL_Window* window_ = nullptr;
    Renderer renderer_;
    Input input_;
    Time time_;
    World world_;
    Entity player_;
};
