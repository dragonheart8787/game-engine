#pragma once

#include <cctype>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include "EntityManager.hpp"
#include "SparseSet.hpp"

namespace ecs {

class World {
public:
    EntityId createEntity() {
        return entity_manager_.create();
    }

    bool destroyEntity(EntityId entity) {
        if (!entity_manager_.isAlive(entity)) {
            return false;
        }

        for (auto& [_, storage] : storages_) {
            storage->removeEntity(entity);
        }

        return entity_manager_.destroy(entity);
    }

    bool isAlive(EntityId entity) const {
        return entity_manager_.isAlive(entity);
    }

    template <typename T>
    T& addComponent(EntityId entity, T component) {
        return storageFor<T>().emplace(entity, std::move(component));
    }

    template <typename T>
    bool removeComponent(EntityId entity) {
        return storageFor<T>().remove(entity);
    }

    template <typename T>
    bool hasComponent(EntityId entity) const {
        return storageFor<T>().has(entity);
    }

    template <typename T>
    T* getComponent(EntityId entity) {
        return storageFor<T>().get(entity);
    }

    template <typename T>
    const T* getComponent(EntityId entity) const {
        return storageFor<T>().get(entity);
    }

    template <typename First, typename... Rest, typename Func>
    void queryAll(Func&& func) {
        auto& base = storageFor<First>();
        for (const EntityId entity : base.denseEntities()) {
            if (!isAlive(entity)) {
                continue;
            }
            if ((storageFor<Rest>().has(entity) && ...)) {
                func(entity, *storageFor<First>().get(entity), *storageFor<Rest>().get(entity)...);
            }
        }
    }

    template <typename T>
    void registerComponentType(
        std::string name,
        std::function<std::string(const T&)> serialize,
        std::function<T(const std::string&)> deserialize) {
        serializers_[name] = [this, name, serialize]() {
            std::string out;
            auto& storage = storageFor<T>();
            bool first = true;
            for (const auto& entity : storage.denseEntities()) {
                if (!isAlive(entity)) {
                    continue;
                }
                if (!first) {
                    out += ",";
                }
                first = false;
                out += "{\"entity\":{";
                out += "\"index\":" + std::to_string(entity.index) + ",\"generation\":" + std::to_string(entity.generation);
                out += "},\"data\":\"" + escapeString(serialize(*storage.get(entity))) + "\"}";
            }
            return out;
        };

        deserializers_[name] = [this, deserialize](EntityId entity, const std::string& payload) {
            addComponent<T>(entity, deserialize(payload));
        };
    }

    std::string serializeWorld() const;
    void deserializeWorld(const std::string& text);

    EntityManager& entityManager() { return entity_manager_; }

    EntityId instantiate(const std::string& prototype_name) {
        auto it = prototypes_.find(prototype_name);
        if (it == prototypes_.end()) {
            throw std::runtime_error("Unknown prototype: " + prototype_name);
        }
        EntityId entity = createEntity();
        for (const auto& attach : it->second) {
            attach(*this, entity);
        }
        return entity;
    }

    template <typename T>
    void addPrototypeComponent(std::string prototype_name, T component) {
        prototypes_[std::move(prototype_name)].push_back([component](World& world, EntityId entity) {
            world.addComponent<T>(entity, component);
        });
    }

private:
    struct IStorage {
        virtual ~IStorage() = default;
        virtual void removeEntity(EntityId entity) = 0;
    };

    template <typename T>
    struct TypedStorage final : IStorage {
        SparseSet<T> storage;

        void removeEntity(EntityId entity) override {
            storage.remove(entity);
        }
    };

    template <typename T>
    SparseSet<T>& storageFor() {
        const std::type_index key = std::type_index(typeid(T));
        auto it = storages_.find(key);
        if (it == storages_.end()) {
            auto created = std::make_unique<TypedStorage<T>>();
            auto* ptr = created.get();
            storages_[key] = std::move(created);
            return ptr->storage;
        }
        return static_cast<TypedStorage<T>*>(it->second.get())->storage;
    }

    template <typename T>
    const SparseSet<T>& storageFor() const {
        const std::type_index key = std::type_index(typeid(T));
        auto it = storages_.find(key);
        if (it == storages_.end()) {
            static const SparseSet<T> empty;
            return empty;
        }
        return static_cast<const TypedStorage<T>*>(it->second.get())->storage;
    }

    static std::string escapeString(const std::string& in) {
        std::string out;
        out.reserve(in.size());
        for (char c : in) {
            if (c == '\\' || c == '"') {
                out.push_back('\\');
            }
            out.push_back(c);
        }
        return out;
    }

    EntityManager entity_manager_;
    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> storages_;
    std::unordered_map<std::string, std::function<std::string()>> serializers_;
    std::unordered_map<std::string, std::function<void(EntityId, const std::string&)>> deserializers_;
    std::unordered_map<std::string, std::vector<std::function<void(World&, EntityId)>>> prototypes_;
};

}  // namespace ecs
