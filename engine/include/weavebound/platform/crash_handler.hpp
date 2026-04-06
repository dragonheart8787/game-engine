#pragma once

namespace weavebound::platform {

/** Minidump（Win）/ signal handler（Linux·Android）占位（規格 1.1 Crash）。 */
class ICrashHandler {
public:
  virtual ~ICrashHandler() = default;
  virtual void install_default_handlers() = 0;
};

}  // namespace weavebound::platform
