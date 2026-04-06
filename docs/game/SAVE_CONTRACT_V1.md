# 存檔與設定契約 v1（WeaveBound ↔ Eidrix）

## 目的

在雙產品線並行下，對齊**語意欄位**（玩家進度、任務步驟、使用者設定），避免「同一欄位名不同義」。二進位格式與 JSON 檔案格式可不同，但**欄位語意**應可對照。

## WeaveBound

### 二進位存檔 `.wbsv`（`save_v0` 命名空間，檔頭 `kFormatVersion`）

| 區塊 | 欄位 | 語意 |
|------|------|------|
| PLAY | health, level, pos_x, pos_y, level_id | 玩家狀態與關卡／場景 id |
| QSTS | active_quest_id, step_index | 任務與步驟（占位，可擴充） |
| WORL | pairs | 世界旗標鍵值（鍵為 `uint32` 四字母 ID） |

解碼實作：[save_game_v0.cpp](../../engine/src/weavebound/game/save_game_v0.cpp)。

### WORL 原型鍵（M1.5）

| 鍵常數 | 值（`save_game_v0.hpp`） | 語意 |
|--------|--------------------------|------|
| `kWorldKeyPrototypeScrap` | `0x53435250`（`'SCRP'`） | WeaveBound 原型**碎片**計數；擊殺取得、可兌換少量 Focus；寫入時值 clamp ≤ 999999。 |

**Eidrix 對照**：進度與「可消耗資源」仍以 **任務／存檔 JSON**（`quest` 狀態等）為主；**不強制**在 Eidrix 寫入上述 WORL 鍵。雙線對齊的是「有取得／消耗決策的迴圈語意」，而非同一整數同步。

### 設定側車 JSON（原型 M1）

- **檔名**：`weavebound_prototype_settings.json`（優先目前工作目錄，其次可執行檔目錄）。
- **欄位**：

```json
{
  "schema_version": "prototype_settings_v1",
  "master_volume": 1.0,
  "sound_enabled": true,
  "tutorial_dismissed": false
}
```

- `master_volume`：0.0–1.0，靜音與音量意圖（引擎無完整混音時可僅作靜音閾值）。
- `sound_enabled`：與系統音效開關一致。

## Eidrix

- **檔名**：`eidrix_settings.json`（套件目錄），見 [settings_store.py](../../../eidrix_mvp/settings_store.py)。
- **欄位對齊**：

| 語意 | WeaveBound JSON | Eidrix JSON |
|------|-----------------|-------------|
| 主音量 | master_volume | volume |
| 靜音／關閉音效 | sound_enabled == false | muted == true |

換算：`muted` 為真時，語意上等價於 `sound_enabled == false` 或 `master_volume == 0`（實作可擇一）。

## 遷移策略

- **v0 → v1**：二進位 chunk 結構未破壞時，舊檔仍可由現有 decode 讀取；新增設定僅經 JSON 側車，無需重寫 `.wbsv`。
- 未來若引入 **save v2**：檔頭 `kFormatVersion` 遞增，並在 [save_game_v0.hpp](../../engine/include/weavebound/game/save_game_v0.hpp) 註記遷移表。

## 關卡／任務 id

- `level_id`（WB PLAY）與 Eidrix `scenario` / `campaign` 關卡 id 應在 [DUAL_LINE_M1.md](../../../eidrix_mvp/docs/DUAL_LINE_M1.md) 維護對照表（M1 起）。
