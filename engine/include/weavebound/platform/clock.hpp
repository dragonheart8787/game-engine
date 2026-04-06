#pragma once

#include <cstdint>
#include <memory>

namespace weavebound::platform {

/** 單調時鐘；實作須避免受系統時間跳躍影響量測。 */
class IClock {
 public:
  virtual ~IClock() = default;
  /** 自實例建立以來的秒（double，用於 frame delta）。 */
  virtual double elapsed_seconds() const = 0;
  /** 平台單調 tick（可選，用於 profiler 對齊）。 */
  virtual std::uint64_t monotonic_ticks() const = 0;
};

/** std::chrono 後端；全平台可用，作為 M0 預設。 */
std::unique_ptr<IClock> create_std_clock();

}  // namespace weavebound::platform
