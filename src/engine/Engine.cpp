#include "engine/Engine.h"

#include <algorithm>
#include <iostream>

namespace {
constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr float kPlayerSpeed = 260.0f;
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

    player_ = world_.createEntity();
    world_.addTransform(player_, Transform{100.0f, 100.0f, 48.0f, 48.0f});
    world_.addVelocity(player_, Velocity{});

    time_.reset(SDL_GetPerformanceCounter());
    isRunning_ = true;
    return true;
}

void Engine::run() {
    while (isRunning_) {
        processInput();
        update();
        render();
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
    input_.beginFrame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        input_.handleEvent(event);
    }

    if (input_.quitRequested()) {
        isRunning_ = false;
    }
}

void Engine::update() {
    const std::uint64_t currentCounter = SDL_GetPerformanceCounter();
    const std::uint64_t counterFrequency = SDL_GetPerformanceFrequency();
    time_.update(currentCounter, counterFrequency);

    Transform* transform = world_.getTransform(player_);
    Velocity* velocity = world_.getVelocity(player_);
    if (!transform || !velocity) {
        return;
    }

    velocity->x = 0.0f;
    velocity->y = 0.0f;

    if (input_.isKeyDown(SDL_SCANCODE_W) || input_.isKeyDown(SDL_SCANCODE_UP)) {
        velocity->y -= kPlayerSpeed;
    }
    if (input_.isKeyDown(SDL_SCANCODE_S) || input_.isKeyDown(SDL_SCANCODE_DOWN)) {
        velocity->y += kPlayerSpeed;
    }
    if (input_.isKeyDown(SDL_SCANCODE_A) || input_.isKeyDown(SDL_SCANCODE_LEFT)) {
        velocity->x -= kPlayerSpeed;
    }
    if (input_.isKeyDown(SDL_SCANCODE_D) || input_.isKeyDown(SDL_SCANCODE_RIGHT)) {
        velocity->x += kPlayerSpeed;
    }

    transform->x += velocity->x * time_.deltaSeconds();
    transform->y += velocity->y * time_.deltaSeconds();
    clampToWindow(*transform);
}

void Engine::render() {
    renderer_.beginFrame();
    if (Transform* transform = world_.getTransform(player_)) {
        renderer_.drawRect(*transform, SDL_Color{66, 135, 245, 255});
    }
    renderer_.endFrame();
}

void Engine::clampToWindow(Transform& transform) {
    transform.x = std::clamp(transform.x, 0.0f, static_cast<float>(kWindowWidth) - transform.width);
    transform.y = std::clamp(transform.y, 0.0f, static_cast<float>(kWindowHeight) - transform.height);
}
