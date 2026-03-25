#pragma once

#include <cstdint>

namespace ecs {

struct EntityId {
    uint32_t index = 0;
    uint32_t generation = 0;

    bool operator==(const EntityId& other) const {
        return index == other.index && generation == other.generation;
    }

    bool operator!=(const EntityId& other) const {
        return !(*this == other);
    }
};

}  // namespace ecs

namespace std {
template <>
struct hash<ecs::EntityId> {
    size_t operator()(const ecs::EntityId& id) const {
        return (static_cast<size_t>(id.generation) << 32U) ^ static_cast<size_t>(id.index);
    }
};
}  // namespace std
