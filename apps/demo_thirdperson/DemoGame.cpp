#include "apps/demo_thirdperson/DemoGame.h"

#include <iostream>

#include <SDL.h>

#include <glm/gtc/matrix_transform.hpp>

namespace apps::demo_thirdperson {

namespace {
constexpr float kMoveSpeed = 4.5f;
constexpr float kDashSpeed = 8.0f;
constexpr float kJumpVelocity = 6.0f;
constexpr float kGravity = -12.0f;
}

bool DemoGame::initialize(engine::core::App& app) {
  app_ = &app;
  input_.initialize();

  if (!renderer_.initialize()) {
    return false;
  }

  camera_.setPerspective(glm::radians(60.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
  debug_.initialize();

  const auto abilityJson = engine::tools::loadJson("assets/abilities/ability_basic.json");
  abilityRuntime_.loadGraph(engine::ability::AbilityGraph::fromJson(abilityJson));

  const auto storyA = engine::tools::loadJson("assets/story/story_a.json");
  director_.loadStory(engine::story::loadStoryAsset(storyA));

  engine::procgen::WorldGenerator generator;
  worldState_ = generator.generate(1337u);

  return true;
}

void DemoGame::update(float deltaSeconds) {
  handleInput();
  updateCharacter(deltaSeconds);
  abilityRuntime_.tick(deltaSeconds);
  for (const auto& event : abilityRuntime_.events()) {
    vfx_.emit(event.type);
  }
  abilityRuntime_.clearEvents();
  updateStory();
  renderer_.hotReload();
}

void DemoGame::render() {
  renderer_.beginFrame({0.1f, 0.1f, 0.12f, 1.0f});

  engine::render::Renderable ground;
  ground.position = {0.0f, -0.01f, 0.0f};
  ground.scale = {10.0f, 1.0f, 10.0f};
  ground.color = {0.2f, 0.7f, 0.3f, 1.0f};

  engine::render::Renderable player;
  player.position = playerPosition_;
  player.scale = {0.6f, 1.2f, 0.6f};
  player.color = {0.2f, 0.5f, 1.0f, 1.0f};

  const engine::math::Vec3 cameraTarget = playerPosition_ + engine::math::Vec3(0.0f, 1.0f, 0.0f);
  const engine::math::Vec3 cameraPos = playerPosition_ + engine::math::Vec3(-3.0f, 3.5f, 6.0f);
  camera_.setView(cameraPos, cameraTarget);

  renderer_.submit(ground, camera_.viewProj());
  renderer_.submit(player, camera_.viewProj());

  renderer_.endFrame();
}

void DemoGame::handleInput() {
  input_.beginFrame();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    input_.handleEvent(event);
  }

  if (!replaying_) {
    recordInput();
  } else {
    replayInput();
  }
}

void DemoGame::updateCharacter(float deltaSeconds) {
  const float speed = input_.isPressed(engine::input::Action::Dash) ? kDashSpeed : kMoveSpeed;
  engine::math::Vec3 move(0.0f);

  if (input_.isPressed(engine::input::Action::MoveForward)) {
    move.z -= speed;
  }
  if (input_.isPressed(engine::input::Action::MoveBackward)) {
    move.z += speed;
  }
  if (input_.isPressed(engine::input::Action::MoveLeft)) {
    move.x -= speed;
  }
  if (input_.isPressed(engine::input::Action::MoveRight)) {
    move.x += speed;
  }

  playerPosition_ += move * deltaSeconds;

  if (onGround_ && input_.isPressed(engine::input::Action::Jump)) {
    playerVelocity_.y = kJumpVelocity;
    onGround_ = false;
  }

  playerVelocity_.y += kGravity * deltaSeconds;
  playerPosition_.y += playerVelocity_.y * deltaSeconds;
  if (playerPosition_.y <= 0.0f) {
    playerPosition_.y = 0.0f;
    playerVelocity_.y = 0.0f;
    onGround_ = true;
  }

  if (input_.isPressed(engine::input::Action::Ability1)) {
    abilityRuntime_.trigger("ability_basic");
  }
}

void DemoGame::updateStory() {
  if (!director_.checkEntryConditions(worldState_.rawJson())) {
    return;
  }
  engine::world::WorldDelta delta = director_.run();
  auto json = worldState_.toJson();
  delta.apply(json);
  worldState_.setJson(json);
}

void DemoGame::recordInput() {
  InputFrame frame;
  frame.forward = input_.isPressed(engine::input::Action::MoveForward);
  frame.backward = input_.isPressed(engine::input::Action::MoveBackward);
  frame.left = input_.isPressed(engine::input::Action::MoveLeft);
  frame.right = input_.isPressed(engine::input::Action::MoveRight);
  frame.jump = input_.isPressed(engine::input::Action::Jump);
  frame.dash = input_.isPressed(engine::input::Action::Dash);
  frame.ability1 = input_.isPressed(engine::input::Action::Ability1);
  recordedInputs_.push_back(frame);
}

void DemoGame::replayInput() {
  if (replayIndex_ >= recordedInputs_.size()) {
    replaying_ = false;
    replayIndex_ = 0;
    return;
  }
  const InputFrame& frame = recordedInputs_[replayIndex_++];
  (void)frame;
}

}  // namespace apps::demo_thirdperson
