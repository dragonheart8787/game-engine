#include "weavebound/scripting/lua_host.hpp"

namespace weavebound::scripting {

namespace {

class LuaHostStub final : public ILuaHost {
 public:
  bool init() override { return true; }
  void shutdown() override {}
};

}  // namespace

std::unique_ptr<ILuaHost> create_lua_host_stub() { return std::make_unique<LuaHostStub>(); }

}  // namespace weavebound::scripting
