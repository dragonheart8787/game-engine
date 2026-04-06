#pragma once

namespace weavebound::net {

/** 多人擴充：snapshot/delta、authority（規格 5；第一版僅介面）。 */
class IReplicationSystem {
public:
  virtual ~IReplicationSystem() = default;
  virtual void tick(double dt) = 0;
};

}  // namespace weavebound::net
