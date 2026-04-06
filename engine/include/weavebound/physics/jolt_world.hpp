#pragma once

namespace weavebound::physics::jolt {

/** 建立 Jolt、靜態地板與動態球（見 ADR 0005）。 */
bool init();

void shutdown();

/** 單次物理步進；未 init 則回傳 false。 */
bool step(float dt_seconds);

/** 自 (0,8,0) 向下射線是否命中靜態幾何（驗證 broad/narrow phase）。 */
bool raycast_down_hit_plane();

}  // namespace weavebound::physics::jolt
