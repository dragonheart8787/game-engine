#pragma once

#include <cstdint>
#include <string_view>

namespace weavebound::observability {

enum class LogLevel : std::uint8_t { Debug, Info, Warn, Error };

/** 分級 log + ring buffer（規格 3.2）。module 可為空。 */
class ILogger {
 public:
  virtual ~ILogger() = default;
  virtual void log(LogLevel level, std::string_view module, std::string_view message) = 0;
};

ILogger* default_logger();

/** 將 ring 內最近紀錄再印一次到 stderr（除錯／smoke）。 */
void dump_recent_logs_to_stderr();

}  // namespace weavebound::observability
