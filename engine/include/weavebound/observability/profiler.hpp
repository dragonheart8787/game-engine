#pragma once

namespace weavebound::observability {

/** CPU zone / GPU timestamp 占位（規格 3.1）。 */
class IProfiler {
 public:
  virtual ~IProfiler() = default;
  virtual void begin_zone(const char* name) = 0;
  virtual void end_zone() = 0;
};

/** 全域 no-op 實作（單執行緒安全於靜態初始化後使用）。 */
IProfiler* default_profiler();

}  // namespace weavebound::observability
