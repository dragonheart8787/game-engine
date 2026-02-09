#pragma once

#include <unordered_map>

#include "engine/Components.h"

struct Entity {
    int id = -1;
};

class World {
public:
    Entity createEntity();
    void destroyEntity(Entity entity);

    void addTransform(Entity entity, const Transform& transform);
    void addVelocity(Entity entity, const Velocity& velocity);

    Transform* getTransform(Entity entity);
    Velocity* getVelocity(Entity entity);

private:
    int nextId_ = 0;
    std::unordered_map<int, Transform> transforms_;
    std::unordered_map<int, Velocity> velocities_;
};
