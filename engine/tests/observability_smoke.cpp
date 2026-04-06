#include "weavebound/observability/frame_meter.hpp"
#include "weavebound/observability/logger.hpp"

int main() {
  using namespace weavebound::observability;
  default_logger()->log(LogLevel::Debug, "obs_smoke", "tick");
  default_logger()->log(LogLevel::Info, "", "no module");
  record_frame_dt_seconds(1.f / 120.f);
  record_frame_dt_seconds(1.f / 60.f);
  const float avg = frame_time_avg_ms_last_n(4);
  const float mx = frame_time_max_ms_last_n(4);
  if (avg <= 0.f || mx <= 0.f) {
    return 1;
  }
  dump_recent_logs_to_stderr();
  return 0;
}
