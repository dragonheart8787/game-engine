#include "engine/ecs/World.h"

namespace engine::ecs {

Entity World::createEntity() {
  return Entity{nextId_++};
}

void World::destroyEntity(Entity entity) {
  (void)entity;
}

}  // namespace engine::ecs
