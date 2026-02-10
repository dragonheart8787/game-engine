#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace engine::core {

struct PlatformConfig {
  std::string title = "Game Engine";
  int width = 1280;
  int height = 720;
  bool useGles = true;
};

enum class KeyCode {
  Unknown,
  W,
  A,
  S,
  D,
  Up,
  Down,
  Left,
  Right,
  Space,
  Shift,
  J,
  K,
  L,
  T,
  Y
};

enum class PlatformEventType {
  None,
  Quit,
  KeyDown,
  KeyUp,
  WindowResize,
  FocusGained,
  FocusLost
};

struct PlatformEvent {
  PlatformEventType type = PlatformEventType::None;
  KeyCode key = KeyCode::Unknown;
  int width = 0;
  int height = 0;
};

class Platform {
public:
  bool initialize(const PlatformConfig& config);
  void shutdown();

  void pollEvents();
  float tick();
  void present();
  void requestQuit();

  bool isRunning() const { return running_; }
  float timeSeconds() const { return timeSeconds_; }
  const std::vector<PlatformEvent>& events() const { return events_; }

private:
  struct SDL_Window* window_ = nullptr;
  void* glContext_ = nullptr;
  bool running_ = false;
  std::uint64_t lastCounter_ = 0;
  float timeSeconds_ = 0.0f;
  std::vector<PlatformEvent> events_;
};

}  // namespace engine::core
