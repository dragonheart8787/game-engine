#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Entity.hpp"

namespace ecs {

template <typename T>
class SparseSet {
public:
    bool has(EntityId entity) const {
        return entity_to_dense_.find(entity.index) != entity_to_dense_.end();
    }

    T& emplace(EntityId entity, T component) {
        auto it = entity_to_dense_.find(entity.index);
        if (it != entity_to_dense_.end()) {
            dense_components_[it->second] = std::move(component);
            return dense_components_[it->second];
        }

        const size_t dense_index = dense_components_.size();
        entity_to_dense_[entity.index] = dense_index;
        dense_entities_.push_back(entity);
        dense_components_.push_back(std::move(component));
        return dense_components_.back();
    }

    bool remove(EntityId entity) {
        auto it = entity_to_dense_.find(entity.index);
        if (it == entity_to_dense_.end()) {
            return false;
        }

        const size_t dense_index = it->second;
        const size_t last_index = dense_components_.size() - 1;

        if (dense_index != last_index) {
            dense_entities_[dense_index] = dense_entities_[last_index];
            dense_components_[dense_index] = std::move(dense_components_[last_index]);
            entity_to_dense_[dense_entities_[dense_index].index] = dense_index;
        }

        dense_entities_.pop_back();
        dense_components_.pop_back();
        entity_to_dense_.erase(it);
        return true;
    }

    T* get(EntityId entity) {
        auto it = entity_to_dense_.find(entity.index);
        if (it == entity_to_dense_.end()) {
            return nullptr;
        }
        return &dense_components_[it->second];
    }

    const T* get(EntityId entity) const {
        auto it = entity_to_dense_.find(entity.index);
        if (it == entity_to_dense_.end()) {
            return nullptr;
        }
        return &dense_components_[it->second];
    }

    const std::vector<EntityId>& denseEntities() const {
        return dense_entities_;
    }

private:
    std::unordered_map<uint32_t, size_t> entity_to_dense_;
    std::vector<EntityId> dense_entities_;
    std::vector<T> dense_components_;
};

}  // namespace ecs
