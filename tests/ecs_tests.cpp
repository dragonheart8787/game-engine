#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>

#include "engine/ecs/World.hpp"

struct Position {
    int x;
    int y;
};

struct Velocity {
    int x;
    int y;
};

static std::string serializePosition(const Position& pos) {
    return std::to_string(pos.x) + "," + std::to_string(pos.y);
}

static Position deserializePosition(const std::string& payload) {
    const size_t comma = payload.find(',');
    return Position{std::stoi(payload.substr(0, comma)), std::stoi(payload.substr(comma + 1))};
}

static std::string serializeVelocity(const Velocity& vel) {
    return std::to_string(vel.x) + "," + std::to_string(vel.y);
}

static Velocity deserializeVelocity(const std::string& payload) {
    const size_t comma = payload.find(',');
    return Velocity{std::stoi(payload.substr(0, comma)), std::stoi(payload.substr(comma + 1))};
}

int main() {
    ecs::World world;
    world.registerComponentType<Position>("Position", serializePosition, deserializePosition);
    world.registerComponentType<Velocity>("Velocity", serializeVelocity, deserializeVelocity);

    // destroy 後舊 handle 失效
    const ecs::EntityId entity = world.createEntity();
    assert(world.isAlive(entity));
    assert(world.destroyEntity(entity));
    assert(!world.isAlive(entity));
    const ecs::EntityId reused = world.createEntity();
    assert(reused.index == entity.index);
    assert(reused.generation == entity.generation + 1);

    // component 正確移除
    world.addComponent<Position>(reused, Position{1, 2});
    assert(world.hasComponent<Position>(reused));
    assert(world.destroyEntity(reused));
    assert(!world.hasComponent<Position>(reused));

    // query 結果正確
    const ecs::EntityId a = world.createEntity();
    const ecs::EntityId b = world.createEntity();
    const ecs::EntityId c = world.createEntity();

    world.addComponent<Position>(a, Position{1, 1});
    world.addComponent<Velocity>(a, Velocity{10, 10});
    world.addComponent<Position>(b, Position{2, 2});
    world.addComponent<Position>(c, Position{3, 3});
    world.addComponent<Velocity>(c, Velocity{30, 30});

    std::unordered_set<uint32_t> ids;
    world.queryAll<Position, Velocity>([&](ecs::EntityId entity_id, Position& pos, Velocity& vel) {
        ids.insert(entity_id.index);
        pos.x += vel.x;
    });

    assert(ids.size() == 2);
    assert(ids.count(a.index) == 1);
    assert(ids.count(c.index) == 1);
    assert(world.getComponent<Position>(a)->x == 11);
    assert(world.getComponent<Position>(b)->x == 2);

    // serialize/deserialize round-trip
    std::string dump = world.serializeWorld();

    ecs::World loaded;
    loaded.registerComponentType<Position>("Position", serializePosition, deserializePosition);
    loaded.registerComponentType<Velocity>("Velocity", serializeVelocity, deserializeVelocity);
    loaded.deserializeWorld(dump);

    size_t loaded_count = 0;
    loaded.queryAll<Position, Velocity>([&](ecs::EntityId, Position& pos, Velocity&) {
        ++loaded_count;
        assert(pos.x == 11 || pos.x == 33);
    });
    assert(loaded_count == 2);

    std::cout << "All ECS tests passed\n";
    return 0;
}
