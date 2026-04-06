# ADR 0004：平台輸入 — Action Map、裝置抽象與 Rebind 資料格式

## 狀態

提案（對齊遊戲層 TDD §6；`platform/input.hpp` 現為型別占位）

## 情境

遊戲玩法需要**邏輯動作**（移動、跳躍、暫停、互動）與實體按鍵／手把軸解耦；設定選單需持久化**自訂綁定**，且未來支援多裝置（鍵盤、滑鼠、手把）。

## 決策（目標架構）

1. **ActionId**：穩定整數或字串 hash（例如 `hash("MoveForward")`），由遊戲或共用表定義；**禁止**在玩法系統直接使用 OS 掃描碼。
2. **ActionMap**：一層或多層（Gameplay、UI、Menu）對應表；每層內 ActionId → 一組 **Binding**（OR 語意：任一觸發即為真）。
3. **Binding**：  
   - **Digital**：`DeviceKind`（Keyboard/MouseButton/GamepadButton）+ `code`（平台中性枚舉或 vendor 擴充欄位）。  
   - **Axis1D / Axis2D**：來源（左搖桿、滑鼠 delta、WASD 合成）+ 死區與曲線（簡化為線性 + clamp）。
4. **裝置抽象**：`platform::IInput` 每幀產出 **RawSnapshot**（按鍵邊緣、軸值）；`InputMapper`（建議放 `engine` 或 game kit）將 RawSnapshot + ActionMap → `ActionState`（pressed/held/released、軸向量）。
5. **Rebind 持久化格式（JSON v0 建議）**：

```json
{
  "schema_version": 1,
  "layers": {
    "gameplay": {
      "MoveForward": [{ "device": "keyboard", "code": "KeyW" }],
      "Pause": [
        { "device": "keyboard", "code": "Escape" },
        { "device": "gamepad", "code": "Start" }
      ]
    }
  }
}
```

- `code` 字串與內部枚舉對照表由引擎維護；遷移時 bump `schema_version`。
- 存檔路徑：使用者設定目錄（與 `SaveGame` 分離，避免與關卡進度混寫）。

## 後果

- **優點**：GDD 主選單／暫停鍵可資料驅動；多人測試與無障礙（重對應）路線清晰。
- **代價**：需在 Win32 訊息迴圈與未來 Raw Input／XInput 之間維護對照表；第一階可只實作鍵盤 + 單一手把按鍵子集。

## 相關檔案

- `engine/include/weavebound/platform/input.hpp`（擴充中）
- `docs/game/TDD.md` §6

---

## MVP：`InputState` 與下一階 ActionMap

本倉庫已提供每幀 **`InputState`**（鍵 held bitmask、滑鼠座標與 `mouse_dx`/`mouse_dy`）與 **`IWindow::read_input`**；Win32 於 `WndProc` 累積，`read_input` 後清零滑鼠增量。下一階將以本快照為 **RawSnapshot**，再經 ADR 決策 §4 之 Mapper 寫入 `ActionState`，並載入 §5 之 JSON 綁定表（無需改動平台訊息泵之核心路徑）。
