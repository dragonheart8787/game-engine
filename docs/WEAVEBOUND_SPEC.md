# WeaveBound 引擎產品規格與路線圖

本文件為 **WeaveBound** 中型 3D 引擎之正式定位、模組邊界與交付階段；並對照本倉庫（[game-engine-platform](https://github.com/dragonheart8787/game-engine)）現狀，供排程與 PR 對齊使用。

---

## 0) 引擎定位與範圍

### 0.1 產品定位

| 項目 | 決策 |
|------|------|
| 引擎類型 | 中型 3D，**單機優先**，保留多人擴充介面 |
| 核心架構 | **Render Graph** + **RHI 抽象** + **ECS / Hybrid ECS** + **資產管線** |
| 渲染路線 | **Phase 1：Forward+**（可擴 **Deferred**） |
| 圖形後端優先序 | **Vulkan**（Win64 / Linux / Android）→ **Metal**（iOS / macOS）→ **D3D12**（Win64 第二階段優化／覆蓋） |

### 0.2 目標平台

| 等級 | 平台 |
|------|------|
| 必須 | Win64 |
| 應支援 | Linux x64、Android、iOS |
| 可選 | Linux arm64（視實際裝置需求） |

---

## 1) 系統層級架構（必做模組）

### 1.1 Platform Layer（平台抽象層）

**目標：** 單一 core 介面涵蓋平台差異。

- **Window / Surface：** Win32、X11、Wayland、Android Native、iOS UIView
- **Input：** Keyboard / Mouse / Gamepad / Touch；支援 **rebind**
- **File I/O：** 虛擬路徑（VFS）、沙盒路徑、串流讀取
- **Time：** 高精度 timer、frame pacing、delta smoothing
- **Threading：** thread + job system（work stealing / queue）
- **System：** CPU/GPU 資訊、記憶體、語系、剪貼簿（可選）
- **Crash：** minidump（Win）/ signal handler（Linux、Android）、callstack

**非功能規格：** 上層**禁止**直接 `#include` 平台標頭；主循環 jitter 可控；輸入延遲可量測。

### 1.2 RHI（Rendering Hardware Interface）

**目標：** 隔離 Vulkan / Metal / D3D12，Renderer 只使用引擎型別。

涵蓋：Buffer / Image / Sampler、Binding 抽象（descriptor set / root signature / Metal argument）、Graphics & Compute PSO、CommandBuffer / Queue / Fence / Semaphore、Swapchain（present、vsync、triple buffering）、staging / ring 上傳、shader 編譯／反射／變體。

**最低要求：** Vulkan（必做）、Metal（必做）、D3D12（第二階段可選）。

### 1.3 Render Graph

**目標：** 自動化 render pass、barrier、resource lifetime。

- Pass / Resource 節點、讀寫依賴邊
- Compile：拓撲排序、lifetime、自動 barrier（stage / access / layout / state）、transient attachment
- Debug：graph dump（JSON）、per-pass GPU markers

**最低要求：** graphics + compute + copy/resolve；最終 post pass + present transition。

### 1.4 Renderer（Phase 1 必做）

- **PBR 基礎：** baseColor、metallic、roughness、normal、AO、emissive
- **光：** Directional（必做）；Point / Spot（應做）
- **陰影：** Directional shadow map（CSM 可選）
- **後處理：** Tone mapping（必做）；Bloom（應做）；FXAA/TAA（可選）
- **Culling：** Frustum（必做）；Occlusion（可選）
- **LOD：** Mesh LOD（應做）
- **GPU Debug：** wireframe / overdraw / normal（可選）

**Phase 2（可選）：** Deferred、SSAO/SSR/Volumetric 等。

### 1.5 ECS / Scene

Hybrid ECS：EntityId、SoA component storage、**可多執行緒**的 system scheduler。

核心 components（示意）：Transform、Hierarchy、Camera、MeshRenderer / SkinnedMeshRenderer、Light、Collider / RigidBody / Character、AudioSource / Listener、ScriptBehaviour。

**Scene：** scene graph、Prefab（應做）、序列化與版本升級。

### 1.6 Asset Pipeline

**Importer：** glTF（必做）、Texture（png/jpg/tga → GPU 格式）、Audio（wav/ogg）。

**Cooker：** mesh optimize、依平台壓縮（BCn / ASTC / ETC2）、mipmap。

**Runtime：** bundle/.pak、依賴圖、hash 快取、**非同步載入**（IO thread + job）、開發模式 hot reload。

**硬規格：** runtime **不可**直接讀 png/jpg；bundle 須支援版本比對與增量替換。

### 1.7 Physics（整合第三方）

Jolt / PhysX / Bullet 擇一；box/sphere/capsule/mesh、static/dynamic/kinematic、Trigger、Raycast/Sweep、Character Controller（應做）；固定步長、transform 同步與插值。

### 1.8 Audio

Mixer、3D spatial（衰減、panning）、串流 BGM；**建議起手：miniaudio**。

### 1.9 UI

- **必做：** Immediate 風格 debug/開發 UI（類 ImGui）
- **選配：** Runtime UI（font atlas、基礎 layout、輸入事件）

### 1.10 Scripting

建議 **Lua**；C++ ↔ Lua binding、entity/component API、事件匯流排、開發模式 hot reload。

---

## 2) 工具鏈與編輯器

### 2.1 CLI（必做）

- `assetc`：import / cook / build bundles  
- `scene_tool`：scene validate、prefab bake、version upgrade  
- `packager`：apk / ipa / zip / appimage  

### 2.2 Editor 分階段

- **v0（必做）：** Headless + JSON/YAML scene  
- **v1（應做）：** 簡易 Scene Editor（viewport、hierarchy、inspector）  
- **v2（可選）：** 完整資產與動畫／材質編輯  

---

## 3) 效能與可觀測性

- **Profiling：** CPU zone、GPU timestamp、per-pass 時間；支援 RenderDoc（Vulkan）/ Xcode GPU Capture（Metal）  
- **Logging：** 分級、ring buffer（避免 IO 卡頓）、GPU debug markers  
- **KPI（可量測）：** 1080p 中階 GPU **60 FPS**、p99 frame time 穩定、asset streaming 不阻塞主執行緒  

---

## 4) 建置、測試、CI

- **CMake** 跨平台  
- 單元測試：core / asset / ecs  
- Render tests：golden image（可選）  
- **CI：** Windows + Linux build & test；Android / iOS 至少 **compile**  

---

## 5) 網路（第一版僅擴充點）

多人時：snapshot/delta replication、authority、可選 client prediction、deterministic-friendly 資料模型。

**第一版不做實作**，僅保留介面（例如 `NetDriver`、`ReplicationSystem`）。

---

## 6) 版本與交付

Save（slot、versioning）、Config（ini/json）、Localization（可選）、bundle 增量 patch（可選）、crash 回報與 symbol。

---

## 7) 本倉庫現狀對照（Gap）

| 規格模組 | 現狀（摘要） | 缺口 |
|----------|----------------|------|
| Platform Layer | `clock` / `window`（Win32 + **`pump_events`**）/ **`InputState` + `IWindow::read_input`（鍵鼠 MVP）** / `input.hpp`；`job_system` inline；`vfs.hpp`、`frame_pacing.hpp` 等 | ActionMap 載入、Raw Input／手把、X11/Wayland、完整 VFS、frame pacing 實測 |
| RHI | Vulkan：`device_vulkan` 可 instance、swapchain、clear、三角形 pipeline、present；`IDevice`；標頭補齊 `pipeline` / `command` / `binding` / `resources` | Metal/D3D12 實作、完整資源生命週期、compute 提交、shader 反射與變體 |
| Render Graph | **M1 進行中**：`ir.hpp` / `RenderGraphBuilder` / `compile()` 拓撲排序 + `BarrierRecord` 占位 + `dump_json()` | 真實 stage/access/layout、transient/alias、per-pass GPU marker |
| Renderer | 無 | Forward+、PBR、陰影、後處理 |
| ECS | C++：`ecs/registry.hpp`（`World` + SoA `Transform3` + 世代銷毀）；Python 仍為工具鏈 | 多執行緒 scheduler、其餘 component、序列化 |
| Asset Pipeline | Python：`pipeline/*`、`bundler`；**新增** `asset/pipeline.hpp` 占位 | glTF cook、runtime 二進位、assetc 實作 |
| Physics / Audio / UI / Lua | **Physics**：Jolt 最小閉環（`physics/jolt_world`、`WEAVEBOUND_WITH_JOLT`、`physics_smoke` raycast）。**Audio**：miniaudio + `IAudioMixer`。**UI／Lua**：stub | 物理 debug draw、音訊 bus／3D、runtime UI、Lua 綁定 |
| 工具 CLI | `cli.py`：**`scene_tool validate --path`**（YAML + `validate_scene`）；**`assetc` / `packager` 占位** | 與 C++ cook/bundle 打通 |
| 可觀測性 / CI | **Logger**：分級＋模組＋ring buffer、`dump_recent_logs_to_stderr`；**frame_meter** 掛 `Application::tick`；pytest + CTest | CPU/GPU profiler、GPU marker、多平台編譯矩陣 |
| 網路 | `net/driver.hpp` 介面占位 | 對齊 §5 之實作 |

**結論：** 規格為**完整商業引擎**等級；本倉庫處於 **M1 Vulkan 可跑 smoke + 介面骨架**階段。實作仍以 **C++ 執行時**為主，Python 負責 **合約／場景驗證／管線占位**。

### 7.1 自動化驗證（本倉庫）

| 類型 | 內容 |
|------|------|
| Python | `pytest tests/test_weavebound_spec_compliance.py`：規格文件關鍵字、`weavebound.hpp` 依賴之標頭檔、Vulkan 原始碼路徑、`scene_tool` / `assetc` / `packager` CLI |
| C++ 編譯 | `weavebound_header_smoke`：僅 `#include <weavebound/weavebound.hpp>`，確保公開 API 標頭可編譯 |
| CTest | `engine_smoke`（預設 `WEAVEBOUND_SMOKE_FRAMES=0` 跳過長時間 present 迴圈）；`weavebound_header_smoke` |

**誠實範圍：** 上表驗證的是「契約與占位在倉庫內可追溯」，**不是** Metal 裝置上跑通、PBR 像素級正確、或 Jolt 物理模擬正確；該類測試需在對應里程碑另建整合／圖像測試。

---

## 8) 建議交付階段（與規格對齊）

### M0 — 地基（4–8 週量級，視人力）

- CMake 多目標（Win64 優先）、目錄切分：`platform/`、`rhi/`、`render_graph/`、`renderer/`（空實作 + 介面）
- Platform：window + 高精度 clock + 單執行緒 job queue 雛型；**編譯隔離** Win32 標頭
- 測試與 CI：Windows + Linux 編譯、核心單元測試

**已著手（本倉庫）：** `engine/include/weavebound/` 下 M0 標頭與 `engine/src/weavebound/` stub；CMake 目標 `weavebound_platform`、`weavebound_rhi`、`weavebound_render_graph`（INTERFACE）、`weavebound_net`（INTERFACE）；`game_engine_core` 鏈結以上模組。

**M1（進行中）：** Win64 + Vulkan SDK：`VkInstance` → Win32 surface → `VkDevice` + swapchain + `VkRenderPass`（clear）→ `VkFramebuffer`／內建 **graphics pipeline**（`triangle.vert`/`frag` → SPIR-V）／`vkQueuePresentKHR`。`IDevice::clear_present_rgba`：清屏後畫三角形。著色器由 CMake `glslc` 編譯，或複製 `engine/shaders/baked/*.spv`；`scripts/bootstrap/` 提供 `download_deps_*` 與 `compile_shaders`。

### M1 — RHI + Render Graph 最小閉環

- Vulkan swapchain + 一個 graphics pass + present
- Render Graph：2–3 pass（clear → triangle/全屏 → present），自動 barrier、JSON dump
- Metal 最小後端（macOS/iOS 編譯管線）

### M2 — Forward+ Phase 1 Renderer

- PBR shader、directional + shadow map、tone mapping、bloom（簡版）
- Frustum culling、mesh LOD（簡版）
- ImGui 或同等 immediate debug UI

### M3 — ECS + Asset Cook + Runtime Load

- SoA storage、scheduler（先單執行緒再擴多執行緒）
- glTF import → cook → runtime 只讀二進位；bundle 版本/hash
- 整合 Jolt 或 Bullet；miniaudio + 基礎 3D audio

### M4+ — D3D12、Deferred、多人介面實作、Editor v1

依產品排程從規格 §0–§6 拉細項。

---

## 9) 與上游倉庫同步建議

若 [dragonheart8787/game-engine](https://github.com/dragonheart8787/game-engine) 為準官方遠端，建議：

1. 將本文件合併至 `main` 並在根目錄 `README.md` 以一小節連結至此規格。  
2. **`docs/ADR/`** 已建立：`0001-rhi-object-model.md`、`0002-render-graph-ir.md`、`0003-ecs-storage.md`；新增不可逆決策時遞增編號。  
3. Issue / Project 以 **M0→M1→…** 里程碑拆解，避免單一 PR 混入全棧（見 **§11**）。

---

## 10) 功能需求清單（FR）索引

| 區塊 | 對應規格 | 本倉庫產物（階段） |
|------|-----------|-------------------|
| **A** Platform | §1.1 | Win32 視窗實作；其餘介面：`vfs` / `frame_pacing` / `delta_smoothing` / `system_info` / `crash_handler` 等標頭 |
| **B** RHI | §1.2 | Vulkan M1；`Backend` 列舉含 Metal/D3D12；`resources` / `pipeline` / `command` / `binding` |
| **C** Render Graph | §1.3 | **`RenderGraphBuilder`**、`compile()`、JSON dump；barrier 為占位 |
| **D** Renderer Phase1 | §1.4 | `renderer/forward_plus_phase1.hpp` 占位 |
| **E** ECS / Scene | §1.5 | `ecs/registry.hpp`（`World`）、`engine/application.hpp`（主循環入口）；ADR 0003 |
| **F** Asset | §1.6 | Python 管線 + `asset/pipeline.hpp`；cook/bundle 實作待 M3 |
| **G** Phys/Audio/UI/Script | §1.7–1.10 | 各 `physics` / `audio` / `ui` / `scripting` 標頭占位 |
| **H** Tools | §2 | `cli.py`：`assetc` / `scene_tool` / `packager`（部分為 stub） |

---

## 11) 非功能規格（NFR）與 Repo 治理（最短落地）

**NFR（精簡）**

- 上層禁止直連平台標頭（編譯隔離）；可觀測性見 `observability/*` 與未來 GPU marker；KPI 與 CI 見 §3、§4、§7.1。

**缺口執行優先序（建議）**

1. Render Graph：barrier 與 layout 對齊 RHI、transient、JSON schema 版本化。  
2. RHI：Buffer/Image/Sampler 建立與上傳、Compute pass、swapchain 三緩衝與 vsync 策略。  
3. Asset：glTF → cook 二進位 → runtime bundle（runtime 不讀 png/jpg）。  
4. Renderer：最小 PBR + directional shadow + tonemap + bloom。  
5. ECS：C++ SoA runtime（避免長期依賴 Python 執行核心）。

**Epic / PR 規則**

- 每個 Epic 對應 **單一里程碑**（M0、M1…）；**禁止**單一 PR 同時改 RHI + RG + Cook + Renderer。  
- 合併前至少：`pytest` 通過；有 CMake 環境則 `ctest`（含 `weavebound_header_smoke`、`engine_smoke`）。

---

## 12) 里程碑驗收標準（DoD，一句話）

| 里程碑 | 可驗收輸出 |
|--------|------------|
| **M0** | 模組目標與 CMake 切分完成；Win64 核心程式可建置執行；CI（或本地）測試可過；不評畫面品質。 |
| **M1** | Vulkan swapchain 閉環；**Render Graph 多 pass IR 可編譯**（拓撲 + JSON + barrier 占位）+ **Metal 最小可編譯**（目標，尚未完成時 Gap 表標示）。 |
| **M2** | Forward+ PBR + directional shadow + tonemap/bloom + frustum cull + debug immediate UI。 |
| **M3** | SoA ECS + glTF cook→bundle→runtime 載入（不讀 png）+ 物理/音效/Lua 基礎可用。 |
| **M4+** | D3D12、Deferred、多人介面實作、Editor v1（依排程）。 |

---

*文件版本：與使用者提供之 WeaveBound 規格對齊；可依實作進度修訂 §7–§8、§10–§12。*
