#include "engine/World.h"

Entity World::createEntity() {
    Entity entity{nextId_++};
    return entity;
}

void World::destroyEntity(Entity entity) {
    transforms_.erase(entity.id);
    velocities_.erase(entity.id);
}

void World::addTransform(Entity entity, const Transform& transform) {
    transforms_[entity.id] = transform;
}

void World::addVelocity(Entity entity, const Velocity& velocity) {
    velocities_[entity.id] = velocity;
}

Transform* World::getTransform(Entity entity) {
    auto it = transforms_.find(entity.id);
    if (it == transforms_.end()) {
        return nullptr;
    }
    return &it->second;
}

Velocity* World::getVelocity(Entity entity) {
    auto it = velocities_.find(entity.id);
    if (it == velocities_.end()) {
        return nullptr;
    }
    return &it->second;
}
