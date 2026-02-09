#include "engine/Engine.h"

#include <iostream>

namespace {
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr float kTargetDelta = 1.0f / 60.0f;
}

bool Engine::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return false;
    }

    window_ = SDL_CreateWindow(
        "Game Engine",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        kWindowWidth,
        kWindowHeight,
        SDL_WINDOW_SHOWN
    );

    if (!window_) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        return false;
    }

    if (!renderer_.initialize(window_)) {
        return false;
    }

    lastCounter_ = SDL_GetPerformanceCounter();
    isRunning_ = true;
    return true;
}

void Engine::run() {
    while (isRunning_) {
        const std::uint64_t currentCounter = SDL_GetPerformanceCounter();
        const std::uint64_t counterFrequency = SDL_GetPerformanceFrequency();
        const float deltaSeconds = static_cast<float>(currentCounter - lastCounter_) /
            static_cast<float>(counterFrequency);
        lastCounter_ = currentCounter;

        processInput();
        update(deltaSeconds);
        render();

        if (deltaSeconds < kTargetDelta) {
            const float delay = (kTargetDelta - deltaSeconds) * 1000.0f;
            SDL_Delay(static_cast<std::uint32_t>(delay));
        }
    }
}

void Engine::shutdown() {
    renderer_.shutdown();

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

void Engine::processInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            isRunning_ = false;
        }
    }
}

void Engine::update(float /*deltaSeconds*/) {
}

void Engine::render() {
    renderer_.beginFrame();
    renderer_.endFrame();
}
