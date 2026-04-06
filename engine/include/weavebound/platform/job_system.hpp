#pragma once

#include <functional>
#include <memory>

#include "weavebound/base/macros.hpp"

namespace weavebound::platform {

/**
 * Job system 抽象；M1+ 可換 work-stealing 實作。
 * M0：inline 單執行緒（submit 立即執行或於 wait_all 排空）。
 */
class IJobSystem : public NonCopyable {
 public:
  virtual ~IJobSystem() = default;
  virtual void submit(std::function<void()> job) = 0;
  virtual void wait_all() = 0;
};

/** 同步執行版：每個 submit 當場跑完。 */
std::unique_ptr<IJobSystem> create_inline_job_system();

}  // namespace weavebound::platform
