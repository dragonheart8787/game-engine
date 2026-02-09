# game-engine

以下內容整理自需求驅動＋最小可行（MVP）方式，讓你可以「真的做得出來」一個專為自己遊戲設計的引擎核心。

## 0) 現實前提：不是技術問題，是範圍控管

自己做引擎最容易失敗的原因是範圍失控。你要做的是「服務你的遊戲」，不是超越 UE/Unity 的通用引擎。
**任何功能如果不能在 2 週內增加可玩的內容，就不做。**

---

## 1) 需求清單：用「不可妥協」寫需求

把需求拆成三層，這比羅列功能更有效。

**A. 不可妥協（Must）**
- 例：3D 第三人稱
- 例：PC 60 FPS
- 例：你的核心戰鬥方式
- 例：小型多人（P2P 或 20 人房間）

**B. 可延後（Should）**
- 破壞（先假破壞：替換 mesh / decal）
- 天氣（先 skybox + fog）
- AI 劇情生成（先固定腳本）

**C. 可以不要（Could）**
- 內建完整編輯器（先 JSON + hot reload）
- 物理精準流體（先用粒子假）

---

## 2) 引擎範圍：只做「你遊戲需要的」

**MVP 必備（真正必須）**
- 遊戲迴圈（TimeStep）
- 視窗 + 輸入
- 渲染（先最簡：mesh + basic material + light）
- 資源管理（texture / mesh / shader）
- 場景管理（load / unload）
- Debug UI（FPS / Draw Calls / entity 數）

**先不做（延後）**
- 完整編輯器（先 JSON + 監看檔案熱載入）
- 完整動畫狀態機（先能播 1 個骨架動畫）
- 真正網路同步（先 local + replay）
- 物理大而全（先 AABB / sphere）

---

## 3) 核心架構：ECS-lite + 模組化

**Engine 層（平台與底層）**
- Platform：視窗、input、檔案、時間
- Renderer：先簡單，RenderGraph 後面再加
- Audio：OpenAL / SDL_mixer
- JobSystem：先不做（先單執行緒）

**Game 層（玩法）**
- World：entity 容器
- Components：Transform、Camera、MeshRenderer、Collider、Script
- Systems：RenderSystem、InputSystem、PhysicsSystem、ScriptSystem

**資料導向 vs 物件導向建議**
- 先 OOP + 資料表最容易做出可玩的 demo
- 等真的卡性能，再進化到完整 ECS/SoA

---

## 4) 技術選型：先能出畫面，不求最潮

**務實入門選型**
- 語言：C++
- 視窗/輸入：SDL2 或 GLFW
- 圖形 API：OpenGL（最快出畫面）
- UI：Dear ImGui（沒有它 debug 會崩潰）
- 資產格式：glTF（模型）、PNG/KTX2（貼圖）

**資料格式**
- MVP：JSON
- 進階：二進位 pack（後面再做）

**編輯器策略**
- scene.json
- assets.json
- hot reload（檔案變更自動 reload）

---

## 5) 實作順序（兩週內能看到成果）

**Phase 0：引擎骨架（1–2 天）**
- window + game loop
- input polling
- ImGui overlay（顯示 FPS / 時間）

**Phase 1：渲染可用（2–4 天）**
- load shader
- draw triangle → draw cube
- camera + basic transform
- 讀一個 glTF（或先 OBJ）

**Phase 2：場景與資源（2–4 天）**
- AssetManager：texture / mesh / shader
- Scene：entity list + transform hierarchy
- Hot reload：shader reload（高 ROI）

**Phase 3：最小互動（3–5 天）**
- player controller（移動 / 視角）
- raycast picking（可指向）
- 碰撞（先 AABB / sphere）

**Phase 4：你的遊戲核心（持續）**
- 先做 1 招、1 個敵人、1 個房間
- 再擴充到 10 招、3 種敵人、1 張地圖

---

## 常見陷阱提醒
- 沒有清楚「不可妥協」需求 → 無限擴張
- 沒有可玩切片 → 引擎做半年還看不到遊戲
- 先做大而全 ECS/Editor → 直接卡死

---

想更進一步的話，把你的**遊戲類型、平台、核心玩法**告訴我，我可以幫你做更精確的模組拆解與技術規劃。
