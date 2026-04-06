#include "weavebound/observability/logger.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace weavebound::observability {

namespace {

constexpr int kRingLines = 256;
constexpr int kLineBytes = 192;

struct RingLine {
  char data[kLineBytes]{};
};

RingLine g_ring[kRingLines]{};
int g_ring_head{0};
int g_ring_count{0};

void append_ring(LogLevel level, std::string_view module, std::string_view message) {
  RingLine& line = g_ring[g_ring_head];
  const char* tag = "INFO";
  if (level == LogLevel::Debug) {
    tag = "DBG ";
  } else if (level == LogLevel::Warn) {
    tag = "WARN";
  } else if (level == LogLevel::Error) {
    tag = "ERR ";
  }
  if (module.empty()) {
    std::snprintf(line.data, kLineBytes, "[%s] %.*s", tag, static_cast<int>(message.size()), message.data());
  } else {
    std::snprintf(line.data, kLineBytes, "[%s][%.*s] %.*s", tag, static_cast<int>(module.size()), module.data(),
                  static_cast<int>(message.size()), message.data());
  }
  line.data[kLineBytes - 1] = '\0';
  g_ring_head = (g_ring_head + 1) % kRingLines;
  g_ring_count = std::min(kRingLines, g_ring_count + 1);
}

class LoggerStderr final : public ILogger {
 public:
  void log(LogLevel level, std::string_view module, std::string_view message) override {
    append_ring(level, module, message);
    const char* tag = "INFO";
    if (level == LogLevel::Debug) {
      tag = "DBG ";
    } else if (level == LogLevel::Warn) {
      tag = "WARN";
    } else if (level == LogLevel::Error) {
      tag = "ERR ";
    }
    if (module.empty()) {
      std::fprintf(stderr, "[%s] %.*s\n", tag, static_cast<int>(message.size()), message.data());
    } else {
      std::fprintf(stderr, "[%s][%.*s] %.*s\n", tag, static_cast<int>(module.size()), module.data(),
                   static_cast<int>(message.size()), message.data());
    }
  }
};

}  // namespace

ILogger* default_logger() {
  static LoggerStderr L{};
  return &L;
}

void dump_recent_logs_to_stderr() {
  if (g_ring_count <= 0) {
    std::fprintf(stderr, "[dump_recent_logs] (empty)\n");
    return;
  }
  const int start = (g_ring_count < kRingLines) ? 0 : g_ring_head;
  std::fprintf(stderr, "--- recent logs (%d) ---\n", g_ring_count);
  for (int i = 0; i < g_ring_count; ++i) {
    const int idx = (start + i) % kRingLines;
    std::fprintf(stderr, "%s\n", g_ring[idx].data);
  }
  std::fprintf(stderr, "--- end ---\n");
}

}  // namespace weavebound::observability
