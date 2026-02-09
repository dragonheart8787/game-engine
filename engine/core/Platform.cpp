#include "engine/core/Platform.h"

#include <iostream>

namespace engine::core {

bool Platform::initialize(const PlatformConfig& config) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  window_ = SDL_CreateWindow(
      config.title.c_str(),
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      config.width,
      config.height,
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  if (!window_) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
    return false;
  }

  glContext_ = SDL_GL_CreateContext(window_);
  if (!glContext_) {
    std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << '\n';
    return false;
  }

  SDL_GL_SetSwapInterval(1);
  running_ = true;
  lastCounter_ = SDL_GetPerformanceCounter();
  return true;
}

void Platform::shutdown() {
  if (glContext_) {
    SDL_GL_DeleteContext(glContext_);
    glContext_ = nullptr;
  }
  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  SDL_Quit();
  running_ = false;
}

float Platform::tick() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    lastEvent_ = event;
    if (event.type == SDL_QUIT) {
      running_ = false;
    }
  }
  const std::uint64_t current = SDL_GetPerformanceCounter();
  const std::uint64_t freq = SDL_GetPerformanceFrequency();
  const float delta = static_cast<float>(current - lastCounter_) / static_cast<float>(freq);
  lastCounter_ = current;
  return delta;
}

void Platform::present() {
  SDL_GL_SwapWindow(window_);
}

void Platform::requestQuit() {
  running_ = false;
}

}  // namespace engine::core
