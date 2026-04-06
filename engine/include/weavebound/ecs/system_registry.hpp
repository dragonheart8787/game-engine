#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "weavebound/base/macros.hpp"
#include "weavebound/ecs/registry.hpp"

namespace weavebound::ecs {

/** 具名 System 依註冊順序執行（M3）。 */
class SystemRegistry : public NonCopyable {
 public:
  using SystemFn = std::function<void(World&)>;

  void register_system(std::string name, SystemFn fn) {
    systems_.push_back(std::make_pair(std::move(name), std::move(fn)));
  }

  void run_all(World& world) {
    for (auto& kv : systems_) {
      kv.second(world);
    }
  }

  std::size_t count() const { return systems_.size(); }

 private:
  std::vector<std::pair<std::string, SystemFn>> systems_;
};

}  // namespace weavebound::ecs
