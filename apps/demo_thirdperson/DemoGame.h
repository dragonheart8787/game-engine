#pragma once

#include <string>
#include <vector>

#include "engine/ability/AbilityRuntime.h"
#include "engine/core/App.h"
#include "engine/debug/DebugOverlay.h"
#include "engine/input/InputSystem.h"
#include "engine/procgen/WorldGenerator.h"
#include "engine/render/CameraSystem.h"
#include "engine/render/Renderer.h"
#include "engine/story/Director.h"
#include "engine/story/IdentityOverride.h"
#include "engine/tools/AssetIO.h"
#include "engine/vfx/VfxSystem.h"
#include "engine/world/WorldDelta.h"
#include "engine/world/WorldHasher.h"
#include "engine/world/WorldState.h"
#include "engine/world/WorldStore.h"

namespace apps::demo_thirdperson {

struct InputFrame {
  bool forward = false;
  bool backward = false;
  bool left = false;
  bool right = false;
  bool jump = false;
  bool dash = false;
  bool ability1 = false;
};

struct InputRecording {
  int version = 1;
  std::uint64_t seed = 0;
  std::vector<InputFrame> frames;
};

class DemoGame {
public:
  bool initialize(engine::core::App& app);
  void beginFrame();
  void handleEvents(const std::vector<engine::core::PlatformEvent>& events);
  void beginRender();
  void fixedUpdate(float deltaSeconds);
  void update(float deltaSeconds);
  void render();
  void endRender();

private:
  void handleInput();
  void updateCharacter(float deltaSeconds);
  void updateStory();
  void recordInput();
  void replayInput();
  bool saveRecording(const std::string& path) const;

  engine::core::App* app_ = nullptr;
  engine::input::InputSystem input_{};
  engine::render::Renderer renderer_{};
  engine::render::CameraSystem camera_{};
  engine::debug::DebugOverlay debug_{};
  engine::ability::AbilityRuntime abilityRuntime_{};
  engine::vfx::VfxSystem vfx_{};

  engine::world::WorldState worldState_;
  engine::story::Director directorA_{};
  engine::story::Director directorB_{};
  engine::story::IdentityOverride identityOverride_{};

  engine::math::Vec3 playerPosition_{0.0f, 0.0f, 0.0f};
  engine::math::Vec3 playerVelocity_{0.0f, 0.0f, 0.0f};
  bool onGround_ = true;

  InputRecording recording_{};
  bool replaying_ = false;
  std::size_t replayIndex_ = 0;
  bool triggerStoryA_ = false;
  bool triggerStoryB_ = false;
};

}  // namespace apps::demo_thirdperson
