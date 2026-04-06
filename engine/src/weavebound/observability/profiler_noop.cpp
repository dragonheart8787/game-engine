#include "weavebound/observability/profiler.hpp"

namespace weavebound::observability {

namespace {

class ProfilerNoop final : public IProfiler {
 public:
  void begin_zone(const char*) override {}
  void end_zone() override {}
};

}  // namespace

IProfiler* default_profiler() {
  static ProfilerNoop p{};
  return &p;
}

}  // namespace weavebound::observability
