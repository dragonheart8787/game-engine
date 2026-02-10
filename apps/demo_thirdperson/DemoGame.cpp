#include "apps/demo_thirdperson/DemoGame.h"

#include <cmath>
#include <fstream>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>

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
  directorA_.loadStory(engine::story::loadStoryAsset(storyA));
  const auto storyB = engine::tools::loadJson("assets/story/story_b.json");
  directorB_.loadStory(engine::story::loadStoryAsset(storyB));

  engine::procgen::WorldGenerator generator;
  worldState_ = generator.generate(1337u);
  recording_.seed = worldState_.getSeed();

  return true;
}

void DemoGame::beginFrame() {
  input_.beginFrame();
}

void DemoGame::handleEvents(const std::vector<engine::core::PlatformEvent>& events) {
  for (const auto& event : events) {
    input_.handleEvent(event);
  }
}

void DemoGame::beginRender() {
  renderer_.beginFrame({0.1f, 0.1f, 0.12f, 1.0f});
}

void DemoGame::fixedUpdate(float deltaSeconds) {
  updateCharacter(deltaSeconds);
  abilityRuntime_.tick(deltaSeconds);
  directorA_.tick(deltaSeconds);
  directorB_.tick(deltaSeconds);
}

void DemoGame::update(float deltaSeconds) {
  handleInput();
  for (const auto& event : abilityRuntime_.events()) {
    vfx_.emit(event.type);
  }
  abilityRuntime_.clearEvents();
  updateStory();

  engine::debug::DebugSnapshot snapshot;
  snapshot.fixedTick = app_ ? app_->fixedTickCount() : 0;
  snapshot.worldHash = engine::world::WorldHasher::hash(worldState_);
  snapshot.deltaCount = worldState_.toJson().value("eventFlags", nlohmann::ordered_json::object()).size();
  snapshot.storyBeat = "runtime";
  snapshot.controlMask = directorA_.controlMask().allowMove ? "move:on" : "move:off";
  debug_.setSnapshot(snapshot);

  renderer_.hotReload();
}

void DemoGame::render() {
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
}

void DemoGame::endRender() {
  renderer_.endFrame();
}

void DemoGame::handleInput() {
  if (!replaying_) {
    recordInput();
  } else {
    replayInput();
  }
}

void DemoGame::updateCharacter(float deltaSeconds) {
  if (!directorA_.controlMask().allowMove || !directorB_.controlMask().allowMove) {
    return;
  }
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

  engine::math::Vec3 direction = move;
  const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y +
                                 direction.z * direction.z);
  if (length > 0.0f) {
    direction /= length;
  }
  abilityRuntime_.setRuntimeParams({{"direction", {direction.x, direction.y, direction.z}},
                                    {"range", 5.0f},
                                    {"width", 1.0f},
                                    {"arc", 30.0f}});

  if (directorA_.controlMask().allowAbility && directorB_.controlMask().allowAbility &&
      input_.isPressed(engine::input::Action::CastAbility1)) {
    abilityRuntime_.trigger("ability_basic", 0);
  }

  if (input_.isPressed(engine::input::Action::TriggerStoryA)) {
    triggerStoryA_ = true;
  }
  if (input_.isPressed(engine::input::Action::TriggerStoryB)) {
    triggerStoryB_ = true;
  }
}

void DemoGame::updateStory() {
  if (triggerStoryA_) {
    if (directorA_.checkEntryConditions(worldState_.rawJson())) {
      engine::world::WorldDelta delta = directorA_.runBeats();
      engine::world::WorldStore::JournalContext ctx;
      ctx.seq = static_cast<std::uint64_t>(recording_.frames.size());
      ctx.tsFixedTick = app_ ? app_->fixedTickCount() : 0;
      ctx.seed = worldState_.getSeed();
      ctx.storyId = "story_a";
      const auto result = engine::world::WorldStore::applyDeltaWithJournal(
          worldState_, delta, "assets/scenes/world_delta_log.jsonl", ctx);
      (void)result;
    }
    triggerStoryA_ = false;
  }
  if (triggerStoryB_) {
    if (directorB_.checkEntryConditions(worldState_.rawJson())) {
      engine::world::WorldDelta delta = directorB_.runBeats();
      engine::world::WorldStore::JournalContext ctx;
      ctx.seq = static_cast<std::uint64_t>(recording_.frames.size());
      ctx.tsFixedTick = app_ ? app_->fixedTickCount() : 0;
      ctx.seed = worldState_.getSeed();
      ctx.storyId = "story_b";
      const auto result = engine::world::WorldStore::applyDeltaWithJournal(
          worldState_, delta, "assets/scenes/world_delta_log.jsonl", ctx);
      (void)result;
    }
    triggerStoryB_ = false;
  }
}

void DemoGame::recordInput() {
  InputFrame frame;
  frame.forward = input_.isPressed(engine::input::Action::MoveForward);
  frame.backward = input_.isPressed(engine::input::Action::MoveBackward);
  frame.left = input_.isPressed(engine::input::Action::MoveLeft);
  frame.right = input_.isPressed(engine::input::Action::MoveRight);
  frame.jump = input_.isPressed(engine::input::Action::Jump);
  frame.dash = input_.isPressed(engine::input::Action::Dash);
  frame.ability1 = input_.isPressed(engine::input::Action::CastAbility1);
  recording_.frames.push_back(frame);
  if (recording_.frames.size() % 300 == 0) {
    saveRecording("assets/scenes/input_recording.json");
  }
}

void DemoGame::replayInput() {
  if (replayIndex_ >= recording_.frames.size()) {
    replaying_ = false;
    replayIndex_ = 0;
    return;
  }
  const InputFrame& frame = recording_.frames[replayIndex_++];
  (void)frame;
}

bool DemoGame::saveRecording(const std::string& path) const {
  nlohmann::ordered_json out;
  out["version"] = recording_.version;
  out["seed"] = recording_.seed;
  out["frames"] = nlohmann::ordered_json::array();
  for (const auto& frame : recording_.frames) {
    out["frames"].push_back({
        {"forward", frame.forward},
        {"backward", frame.backward},
        {"left", frame.left},
        {"right", frame.right},
        {"jump", frame.jump},
        {"dash", frame.dash},
        {"ability1", frame.ability1}
    });
  }

  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }
  file << out.dump(2);
  return true;
}
}  // namespace apps::demo_thirdperson
