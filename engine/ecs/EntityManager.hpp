#pragma once

#include <cstdint>
#include <vector>

#include "Entity.hpp"

namespace ecs {

class EntityManager {
public:
    EntityId create() {
        if (!free_indices_.empty()) {
            const uint32_t index = free_indices_.back();
            free_indices_.pop_back();
            alive_[index] = true;
            return EntityId{index, generations_[index]};
        }

        const uint32_t index = static_cast<uint32_t>(generations_.size());
        generations_.push_back(0);
        alive_.push_back(true);
        return EntityId{index, 0};
    }

    bool destroy(EntityId id) {
        if (!isAlive(id)) {
            return false;
        }

        alive_[id.index] = false;
        ++generations_[id.index];
        free_indices_.push_back(id.index);
        return true;
    }

    bool isAlive(EntityId id) const {
        return id.index < generations_.size() && alive_[id.index] && generations_[id.index] == id.generation;
    }

    std::vector<EntityId> aliveEntities() const {
        std::vector<EntityId> out;
        for (uint32_t i = 0; i < generations_.size(); ++i) {
            if (alive_[i]) {
                out.push_back(EntityId{i, generations_[i]});
            }
        }
        return out;
    }

private:
    std::vector<uint32_t> generations_;
    std::vector<bool> alive_;
    std::vector<uint32_t> free_indices_;
};

}  // namespace ecs
