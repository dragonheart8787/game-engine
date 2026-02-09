#pragma once

#include <SDL.h>
#include <string>

namespace engine::core {

struct PlatformConfig {
  std::string title = "Game Engine";
  int width = 1280;
  int height = 720;
  bool useGles = true;
};

class Platform {
public:
  bool initialize(const PlatformConfig& config);
  void shutdown();

  float tick();
  void present();
  void requestQuit();

  bool isRunning() const { return running_; }
  SDL_Window* window() const { return window_; }
  SDL_GLContext glContext() const { return glContext_; }

  const SDL_Event& lastEvent() const { return lastEvent_; }

private:
  SDL_Window* window_ = nullptr;
  SDL_GLContext glContext_ = nullptr;
  bool running_ = false;
  std::uint64_t lastCounter_ = 0;
  SDL_Event lastEvent_{};
};

}  // namespace engine::core
