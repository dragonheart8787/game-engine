#include "weavebound/platform/job_system.hpp"

#include <memory>

namespace weavebound::platform {

namespace {

class InlineJobSystem final : public IJobSystem {
 public:
  void submit(std::function<void()> job) override {
    if (job) {
      job();
    }
  }

  void wait_all() override {}
};

}  // namespace

std::unique_ptr<IJobSystem> create_inline_job_system() { return std::make_unique<InlineJobSystem>(); }

}  // namespace weavebound::platform
