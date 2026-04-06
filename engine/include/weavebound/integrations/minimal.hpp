#pragma once

namespace weavebound::integrations {

/** Jolt／Bullet 接入前的最小閉環占位；`fixed_dt_seconds` 與 `Application::fixed_update` 步長一致。 */
bool physics_step_minimal(float fixed_dt_seconds = 1.f / 60.f);

/** miniaudio 接入前的最小閉環占位。 */
bool audio_tick_minimal();

/** Lua host 接入前的最小閉環占位。 */
bool lua_host_smoke();

}  // namespace weavebound::integrations
