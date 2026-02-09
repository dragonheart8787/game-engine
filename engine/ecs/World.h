#pragma once

#include <unordered_map>
#include <vector>

namespace engine::ecs {

struct Entity {
  int id = -1;
};

class World {
public:
  Entity createEntity();
  void destroyEntity(Entity entity);

  template <typename T>
  void addComponent(Entity entity, const T& component) {
    storage<T>()[entity.id] = component;
  }

  template <typename T>
  T* getComponent(Entity entity) {
    auto& store = storage<T>();
    auto it = store.find(entity.id);
    if (it == store.end()) {
      return nullptr;
    }
    return &it->second;
  }

  template <typename T>
  std::unordered_map<int, T>& storage() {
    static std::unordered_map<int, T> store;
    return store;
  }

private:
  int nextId_ = 0;
};

}  // namespace engine::ecs
