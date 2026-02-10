#include "engine/core/Platform.h"

#include <iostream>

#include <SDL.h>

namespace engine::core {

namespace {
KeyCode toKeyCode(SDL_Scancode scancode) {
  switch (scancode) {
    case SDL_SCANCODE_W:
      return KeyCode::W;
    case SDL_SCANCODE_A:
      return KeyCode::A;
    case SDL_SCANCODE_S:
      return KeyCode::S;
    case SDL_SCANCODE_D:
      return KeyCode::D;
    case SDL_SCANCODE_UP:
      return KeyCode::Up;
    case SDL_SCANCODE_DOWN:
      return KeyCode::Down;
    case SDL_SCANCODE_LEFT:
      return KeyCode::Left;
    case SDL_SCANCODE_RIGHT:
      return KeyCode::Right;
    case SDL_SCANCODE_SPACE:
      return KeyCode::Space;
    case SDL_SCANCODE_LSHIFT:
      return KeyCode::Shift;
    case SDL_SCANCODE_J:
      return KeyCode::J;
    case SDL_SCANCODE_K:
      return KeyCode::K;
    case SDL_SCANCODE_L:
      return KeyCode::L;
    case SDL_SCANCODE_T:
      return KeyCode::T;
    case SDL_SCANCODE_Y:
      return KeyCode::Y;
    default:
      return KeyCode::Unknown;
  }
}
}  // namespace

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
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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

void Platform::pollEvents() {
  events_.clear();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    PlatformEvent ev{};
    switch (event.type) {
      case SDL_QUIT:
        ev.type = PlatformEventType::Quit;
        events_.push_back(ev);
        running_ = false;
        break;
      case SDL_KEYDOWN:
        ev.type = PlatformEventType::KeyDown;
        ev.key = toKeyCode(event.key.keysym.scancode);
        events_.push_back(ev);
        break;
      case SDL_KEYUP:
        ev.type = PlatformEventType::KeyUp;
        ev.key = toKeyCode(event.key.keysym.scancode);
        events_.push_back(ev);
        break;
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          ev.type = PlatformEventType::WindowResize;
          ev.width = event.window.data1;
          ev.height = event.window.data2;
          events_.push_back(ev);
        } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
          ev.type = PlatformEventType::FocusGained;
          events_.push_back(ev);
        } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
          ev.type = PlatformEventType::FocusLost;
          events_.push_back(ev);
        }
        break;
            case SDL_FINGERDOWN:
        ev.type = PlatformEventType::TouchDown;
        ev.touchX = event.tfinger.x;
        ev.touchY = event.tfinger.y;
        events_.push_back(ev);
        break;
      case SDL_FINGERMOTION:
        ev.type = PlatformEventType::TouchMove;
        ev.touchX = event.tfinger.x;
        ev.touchY = event.tfinger.y;
        events_.push_back(ev);
        break;
      case SDL_FINGERUP:
        ev.type = PlatformEventType::TouchUp;
        ev.touchX = event.tfinger.x;
        ev.touchY = event.tfinger.y;
        events_.push_back(ev);
        break;
      default:
        break;
    }
  }
}

float Platform::tick() {
  const std::uint64_t current = SDL_GetPerformanceCounter();
  const std::uint64_t freq = SDL_GetPerformanceFrequency();
  const float delta = static_cast<float>(current - lastCounter_) / static_cast<float>(freq);
  lastCounter_ = current;
  timeSeconds_ += delta;
  return delta;
}

void Platform::present() {
  SDL_GL_SwapWindow(window_);
}

void Platform::requestQuit() {
  running_ = false;
}

}  // namespace engine::core
