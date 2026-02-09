#include "engine/Renderer.h"

#include <iostream>

#include "engine/Components.h"

bool Renderer::initialize(SDL_Window* window) {
    renderer_ = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer_) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
        return false;
    }

    return true;
}

void Renderer::beginFrame() {
    SDL_SetRenderDrawColor(renderer_, 10, 12, 20, 255);
    SDL_RenderClear(renderer_);
}

void Renderer::drawRect(const Transform& transform, SDL_Color color) {
    SDL_Rect rect{
        static_cast<int>(transform.x),
        static_cast<int>(transform.y),
        static_cast<int>(transform.width),
        static_cast<int>(transform.height)
    };
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer_, &rect);
}

void Renderer::endFrame() {
    SDL_RenderPresent(renderer_);
}

void Renderer::shutdown() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
}
