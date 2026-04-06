#pragma once

namespace weavebound::observability {

/** 記錄主循環 frame dt（秒）；保留最近 128 幀。 */
void record_frame_dt_seconds(float dt_seconds);

/** 最近 n 幀平均耗時（毫秒）；n 夾在 1..128。 */
float frame_time_avg_ms_last_n(int n);

/** 最近 n 幀最大耗時（毫秒）。 */
float frame_time_max_ms_last_n(int n);

}  // namespace weavebound::observability
