#pragma once

namespace weavebound::platform {

/** 主循環 delta 平滑（規格 1.1 Time；與 clock、frame_pacing 並列）。 */
class IDeltaTimeSmoother {
public:
  virtual ~IDeltaTimeSmoother() = default;
  virtual void push_raw_seconds(double dt_raw) = 0;
  virtual double smoothed_seconds() const = 0;
};

}  // namespace weavebound::platform
