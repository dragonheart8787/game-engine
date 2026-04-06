#pragma once

// 最小系統音效（Windows MessageBeep），可關閉；無外部音檔依賴。
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace weavebound::prototype_sound {

inline bool g_enabled = true;
/** 0–1，見 docs/game/SAVE_CONTRACT_V1.md（無系統音量 API 時仍作靜音閾值）。 */
inline float g_master_volume = 1.f;

inline void toggle() { g_enabled = !g_enabled; }

inline void set_master_volume(float v) {
  g_master_volume = (v < 0.f) ? 0.f : (v > 1.f ? 1.f : v);
}

inline float master_volume() { return g_master_volume; }

inline bool sound_effective() { return g_enabled && g_master_volume > 1e-4f; }

inline void ping_light() {
#if defined(_WIN32)
  if (sound_effective()) {
    MessageBeep(0xFFFFFFFF);  // 系統預設
  }
#endif
}

inline void ping_alert() {
#if defined(_WIN32)
  if (sound_effective()) {
    MessageBeep(MB_ICONASTERISK);
  }
#endif
}

}  // namespace weavebound::prototype_sound
