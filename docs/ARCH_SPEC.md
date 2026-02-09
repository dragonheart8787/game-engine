# ARCH_SPEC

本文件僅落地既有規格與資料格式，不新增架構或改動模組邊界。

## 目標
- 12 週內完成可運行骨架（PC + Mobile）
- 3D 第三人稱 Demo（角色移動、相機、基礎戰鬥/能力 Graph v0、基本 AI、基本場景）
- WorldState/WorldDelta deterministic（一致性 hash）
- Story 系統最小 runtime（Identity Overwrite + Director beats）
- 資產載入（glTF/texture/audio）與 hot reload（shader + json）

## 模組邊界（固定）
- engine/core: Platform, App, time, fs, logging, config, job stub
- engine/math: glm wrappers, deterministic random
- engine/ecs: entity, component storage, queries
- engine/world: WorldState, WorldDelta, persistence, hashing
- engine/render: renderer, materials, mesh, camera, post fx stub
- engine/vfx: trail/beam/particle stub
- engine/input: mapping, rebinding, action system
- engine/audio: miniaudio wrapper
- engine/physics: raycast, sweep
- engine/ai: behavior stub + state machine
- engine/net: replication interface only
- engine/story: Identity Overwrite + Director beats runtime
- engine/procgen: seeded world/biome layout stub
- engine/ability: Ability Graph v0 runtime + serialization
- engine/tools: shared code for tools
- engine/debug: imgui overlay (PC), mobile debug log
- engine/platform: win/linux/android/ios stub

## API 契約（名稱固定）
- engine::core::App (run loop)
- engine::core::Platform (window/input/time/fs abstraction)
- engine::world::WorldState (immutable read API)
- engine::world::WorldDelta (apply/merge)
- engine::world::WorldStore (save/load)
- engine::world::WorldHasher (stable hash)
- engine::ecs::World (entities/components)
- engine::input::InputSystem (actions + mapping + rebind)
- engine::render::Renderer (init/frame/submit)
- engine::render::CameraSystem
- engine::ability::AbilityGraph (load/execute)
- engine::ability::AbilityRuntime (tick/emit events)
- engine::story::Director (beats runner)
- engine::story::IdentityOverride (apply/revert)
- engine::procgen::WorldGenerator (seed -> WorldState + scene spawns)
- engine::debug::DebugOverlay (PC only)
- apps/demo_thirdperson::DemoGame (glue code)

## 資料格式
### WorldState JSON
- 位置: assets/scenes/worldstate.schema.json / assets/scenes/worldstate.json
- 欄位：seed, regions[], cityState, characters[], eventFlags

### WorldDelta JSON
- 位置: assets/scenes/worlddelta.json
- ops: set/add/inc/remove

### AbilityGraph JSON
- 位置: assets/abilities/*.json
- nodes[]: id, type, params{}
- edges[]: from, to
- Node types v0: Shape, Path, Constraint, Spawn, Affect, Cost, Cooldown, Interact

### StoryAsset JSON
- 位置: assets/story/*.json
- storyId, mode(A/B), overrideType(Full/Partial/Lens)
- entryConditions, beats[], constraints, worldDeltaOut

## Determinism
- 固定 seed + 同輸入回放 → 相同 WorldState hash
- Hash 規則：使用 ordered_json dump 的 FNV-1a

## Demo
- 視窗開啟 + 渲染場景
- 角色 Move/Look/Jump/Dash + 碰撞 (capsule vs ground stub)
- AbilityGraph v0 觸發 → VFX stub
- Story 觸發 → WorldDelta apply
