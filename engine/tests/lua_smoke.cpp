#include "weavebound/scripting/lua_host.hpp"

int main() {
  auto h = weavebound::scripting::create_lua_host_stub();
  if (!h || !h->init()) {
    return 1;
  }
  h->shutdown();
  return 0;
}
