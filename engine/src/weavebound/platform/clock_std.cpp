#include "weavebound/platform/clock.hpp"

#include <chrono>
#include <memory>

namespace weavebound::platform {

namespace {

class StdClock final : public IClock {
 public:
  StdClock() : start_(std::chrono::steady_clock::now()) {}

  double elapsed_seconds() const override {
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - start_).count();
  }

  std::uint64_t monotonic_ticks() const override {
    const auto now = std::chrono::steady_clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count());
  }

 private:
  std::chrono::steady_clock::time_point start_;
};

}  // namespace

std::unique_ptr<IClock> create_std_clock() { return std::make_unique<StdClock>(); }

}  // namespace weavebound::platform
