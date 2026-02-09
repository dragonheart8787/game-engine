#pragma once

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

class DemoGame {
public:
  bool initialize(engine::core::App& app);
  void update(float deltaSeconds);
  void render();

private:
  void handleInput();
  void updateCharacter(float deltaSeconds);
  void updateStory();
  void recordInput();
  void replayInput();

  engine::core::App* app_ = nullptr;
  engine::input::InputSystem input_{};
  engine::render::Renderer renderer_{};
  engine::render::CameraSystem camera_{};
  engine::debug::DebugOverlay debug_{};
  engine::ability::AbilityRuntime abilityRuntime_{};
  engine::vfx::VfxSystem vfx_{};

  engine::world::WorldState worldState_;
  engine::story::Director director_{};
  engine::story::IdentityOverride identityOverride_{};

  engine::math::Vec3 playerPosition_{0.0f, 0.0f, 0.0f};
  engine::math::Vec3 playerVelocity_{0.0f, 0.0f, 0.0f};
  bool onGround_ = true;

  std::vector<InputFrame> recordedInputs_;
  bool replaying_ = false;
  std::size_t replayIndex_ = 0;
};

}  // namespace apps::demo_thirdperson
