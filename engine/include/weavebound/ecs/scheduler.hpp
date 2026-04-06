#pragma once

#include <cstddef>
#include <initializer_list>

namespace weavebound::ecs {

class World;

using SystemFn = void (*)(World&);

/** 單執行緒有序執行（之後可換 job DAG）。 */
inline void run_systems(World& w, std::initializer_list<SystemFn> systems) {
  for (SystemFn s : systems) {
    if (s) {
      s(w);
    }
  }
}

}  // namespace weavebound::ecs
