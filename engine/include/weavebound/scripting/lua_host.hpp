#pragma once

#include <memory>

namespace weavebound::scripting {

/** Lua VM + 綁定 + hot reload 開發模式占位（規格 1.10）。 */
class ILuaHost {
 public:
  virtual ~ILuaHost() = default;
  virtual bool init() = 0;
  virtual void shutdown() = 0;
};

/** 無內嵌 VM 之占位（之後可換 sol2 + Lua 動態庫）。 */
std::unique_ptr<ILuaHost> create_lua_host_stub();

}  // namespace weavebound::scripting
