#pragma once

#include <cstdint>

#include "weavebound/rhi/types.hpp"

namespace weavebound::rhi {

/** CommandBuffer / Fence / Semaphore 占位（規格 1.2 Command）。 */
class ICommandBuffer {
 public:
  virtual ~ICommandBuffer() = default;
  virtual void begin() = 0;
  virtual void end() = 0;
};

class IFence {
 public:
  virtual ~IFence() = default;
  virtual void reset() = 0;
  virtual bool wait_ns(std::uint64_t timeout_ns) = 0;
};

class ISemaphore {
 public:
  virtual ~ISemaphore() = default;
};

}  // namespace weavebound::rhi
