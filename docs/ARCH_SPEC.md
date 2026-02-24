# ARCH_SPEC

本文件為「最終版模組與 Demo 串接規格」，僅落地規格，不新增架構或改動模組邊界。

## 1) 核心架構與入口（Core / App / Platform）

**入口點**：`apps/demo_thirdperson/main.cpp`

- 必須只做：建立 `engine::core::App`、註冊 `DemoGame`、呼叫 `App::run()`。

**App 職責：`engine::core::App`**

- 管理固定更新步長（FixedUpdate）與變步長渲染（Update/Render）。
- 更新順序必須固定：

```
Platform::PollEvents() → InputSystem::BeginFrame() → Game::Update() → Renderer::BeginFrame() →
Game::Render() → Renderer::EndFrame() → Platform::Present()
```

- 必須提供：`run()` / `requestQuit()` / `timeSeconds()` / `platform()`。

**Platform 職責：`engine::core::Platform`**

- SDL2 建立視窗、事件、時間、檔案 IO 抽象。
- 建立 OpenGL ES 3.x context（PC 以 OpenGL 或 ANGLE 兼容層皆可，但對上層暴露為 GLES3 API）。
- 事件處理：提供 `pollEvents()` 並回報 quit、resize、focus 事件。

**硬條件**
- 任何模組不得直接 include SDL headers，僅 platform 模組可碰 SDL。
- Renderer 必須能在 “no assets” 模式下跑起來（空場景也不 crash）。

## 2) 渲染系統（Render / Camera）

**`engine::render::Renderer`**

- 最小 GLES3 pipeline：VBO/IBO、基本 shader、uniform 管理。
- 支援 shader hot-reload：檔案變更後 3 秒內生效（PC）。
- 提供渲染提交介面：
  - `submit(const Renderable&)`
  - `submitDebug(const DebugDrawCmd&)`（VFX stub 可用）

**`engine::render::CameraSystem`**

- 生成 view/proj matrix（3rd person follow camera）。
- 提供相機控制權遮罩（Story 用）：
  - `setControlMask(CameraControlMask mask)`
  - mask 至少包含：`Follow`、`FreeLook`、`CinematicLock`

**硬條件**
- 任何 Renderable 必須只包含 POD/handle（避免跨模組 ownership 問題）。
- hot-reload 失敗必須 fallback 到舊 shader，不得黑屏。

## 3) WorldState / WorldDelta / Hash（決定性一致性核心）

**`engine::world::WorldState`**

- 從 JSON 載入（`nlohmann::ordered_json`），並提供唯讀 query API：
  - `getSeed()`
  - `findRegion(id)`
  - `getCharacter(id)`

**`engine::world::WorldDelta`**

- 支援 patch operations：`set/add/inc/remove`
- `apply(WorldStateMutable&)` 交易式套用（apply 失敗要返回 error）
- `merge(const WorldDelta&, ConflictPolicy)`：至少支援 `LastWriteWins` 與 `RejectOnConflict`

**`engine::world::WorldHasher`**

- 使用 ordered_json dump + FNV-1a 生成穩定 hash
- hash 規則：
  - 所有浮點在 dump 前需 quantize（例如保留 1e-4）以避免平台差異
  - 必須固定 UTF-8、固定 key order

**硬條件**
- deterministic 驗證：相同 seed + 相同 input recording replay → WorldHasher 相同（tests 必須覆蓋）。
- JSON parse 失敗要回傳「path + reason」（至少 key path）。

## 4) Ability / VFX（能力圖與事件）

**`engine::ability::AbilityGraph`**

- 解析 AbilityGraph JSON：nodes/edges
- Node v0 支援：`Shape`、`Path`、`Constraint`、`Spawn`、`Affect`、`Cost`、`Cooldown`、`Interact`
- 必須支援 runtime param override（來自玩家操作）：
  - `direction`、`width`、`arc`、`range`

**`engine::ability::AbilityRuntime`**

- Tick 執行 graph → 產生事件（事件 schema 固定）：
  - `SpawnTrail`
  - `SpawnBeam`
  - `ApplyImpulse`
  - `ApplyStatus`
- 事件帶上：`AbilityId`、`EmitterEntityId`、`Transform`、`Params`

**`engine::vfx`**

- v0 可用 DebugDraw 代替（線條/帶狀），但接口必須存在，後續可替換成粒子。

**硬條件**
- AbilityRuntime 不得直接呼叫 Renderer；只能 emit events。
- Cooldown/Cost 記帳必須是 deterministic（不得用非決定性時間源）。

## 5) Story / Director / IdentityOverride（電影感 runtime 骨架）

**`engine::story::Director`**

- 載入 StoryAsset JSON（mode A/B, overrideType Full/Partial/Lens）
- `runBeats()`：依 beats 執行
- 必須支援 ControlMode：
  - `Hold` / `Guide` / `Punch`
- 必須支援「權限遮罩」：
  - 可獨立禁止：Move / Look / Ability / UI

**`engine::story::IdentityOverride`**

- apply/revert：
  - Full：替換身份資料（inventory/flags/relations stub）
  - Partial：加上限制（exposure meter / ability limit）
  - Lens：只提供視角/資訊，不變更本體能力（stub）

**硬條件**
- Story 不能長時間剝奪控制權：Hold 必須有最大上限（例如 2 秒）。
- 所有 story 觸發條件必須只依賴 WorldState（避免資料漂移）。

## 6) Demo 串接（apps/demo_thirdperson）

**DemoGame 負責**

- 初始化：載入 worldstate.json、建立簡單 scene（地面 + 角色 cube）
- 更新：Input 驅動角色控制；按鍵觸發 Ability；Story 條件達成時跑 Director
- 渲染：Renderer submit 地面/角色；相機跟隨

**Input mapping 最少包含**

- Move、Look、Jump、Dash、CastAbility1、TriggerStoryA、TriggerStoryB、ToggleDebug

**硬條件**

- Demo 必須在「只有 assets 樣例」的情況下可跑（不依賴外部工具）。
- Android Demo：觸控映射（虛擬搖桿 stub + 可配置）。

## 7) CMake / Targets / Third-party / Testing

- 每個模組一個 target（static library）：
  - `engine_core`, `engine_render`, `engine_world`, `engine_ability`, `engine_story`...
- `apps/demo_thirdperson` link 需要的 targets
- third party 只允許 FetchContent（glm, ordered_json, stb, miniaudio, SDL2）
- tests：
  - `tests/worldstate_tests` 覆蓋：load/hash/delta/merge/replay

**硬條件**
- CI 必須至少提供：
  - Windows build
  - Linux build
  - Android build（可只 build 不跑）
- 格式化：clang-format + 可選 pre-commit hook

## 補強項（已落地）
- Determinism guardrails：固定 tick、DeterministicRng、versioned input recording。
- WorldDelta：validation + transactional apply + journaling（`world_delta_log`）。
- Android input stub：觸控事件映射進同一套 Action system。
- CI 規則：SDL include isolation check。
- 資產驗證工具：`tools/validate_assets`。

## 補強項（已落地）
- Offline-first 依賴策略：`ENGINE_VENDOR_DEPS=ON` 時不允許 FetchContent；優先 system/third_party。
- Determinism stream policy：`WorldGen=1`, `AI=2`, `Ability=3`, `Story=4`（預留）。
- Golden hash 測試：`tests/worldstate_tests` 比對固定 hash。
- Journal 格式：`journal_v1.jsonl`，欄位 `seq/ts_fixedTick/seed/storyId/deltaHash/deltaOps/result/message`。
- Journal 輪替：依最大筆數（`maxEntries`）保留最新紀錄。
- Android 觸控 stub：touch 事件統一映射 Action system，並有 `touch_mapping.json` 配置。
- 資產驗證工具：Ability cycle/參數/邊緣引用、Story beat 欄位、WorldDelta path/value 基本驗證。
- Replay diff 工具：輸出第一個 divergence tick + system + streamId + delta_seq。

## 交付品質 Gate（新增）
- 資產管線：`asset_packer` 產出 deterministic manifest（sorted entries + FNV-1a）。
- 可重現建置：`ENGINE_VENDOR_DEPS=ON` + `check_vendor_deps.sh` 必須通過。
- 性能回歸：`perf_regression_check.py` 以 baseline/tolerance 驗證。
- 記憶體預算：`memory_budget_check.py` 驗證副檔名/總量 budget。
- 觀測：`observability_check.py` 驗證 telemetry 必備欄位。

## Render backend 心智模型（Vulkan / D3D12 / Metal）
- 統一抽象：RenderPassDescription / GraphicsPipelineDescription / ResourceTransition。
- Vulkan 對應：`VkRenderPass` + `AttachmentDescription` + `Subpass`。
- D3D12 對應：`OMSetRenderTargets`（現行）+ `BeginRenderPass`（後續）+ `ResourceBarrier Transition`。
- Metal 對應：`MTLRenderPassDescriptor` + `MTLRenderPipelineState` + `MTLDepthStencilState`。
- Pipeline State 固定欄位：Shader(VS/PS/CS), Input Layout, Blend, Depth, Rasterizer, Topology, RT Format。
