#pragma once

namespace weavebound::net {

/**
 * 多人連線擴充點（規格 §5）：第一版不實作同步，僅保留介面形狀。
 */
class INetDriver {
 public:
  virtual ~INetDriver() = default;
  virtual bool start() = 0;
  virtual void stop() = 0;
};

}  // namespace weavebound::net
