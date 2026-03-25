#include "World.hpp"

#include <algorithm>
#include <unordered_map>

namespace ecs {
namespace {

struct Parser {
    explicit Parser(const std::string& input) : text(input) {}

    const std::string& text;
    size_t pos = 0;

    void skipWs() {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
    }

    bool consume(char c) {
        skipWs();
        if (pos < text.size() && text[pos] == c) {
            ++pos;
            return true;
        }
        return false;
    }

    void expect(char c) {
        if (!consume(c)) {
            throw std::runtime_error("JSON parse error");
        }
    }

    std::string parseString() {
        skipWs();
        expect('"');
        std::string out;
        while (pos < text.size()) {
            char c = text[pos++];
            if (c == '"') {
                return out;
            }
            if (c == '\\') {
                if (pos >= text.size()) {
                    throw std::runtime_error("JSON escape error");
                }
                out.push_back(text[pos++]);
                continue;
            }
            out.push_back(c);
        }
        throw std::runtime_error("Unterminated string");
    }

    uint32_t parseUInt() {
        skipWs();
        size_t start = pos;
        while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
        if (start == pos) {
            throw std::runtime_error("Expected integer");
        }
        return static_cast<uint32_t>(std::stoul(text.substr(start, pos - start)));
    }
};

}  // namespace

std::string World::serializeWorld() const {
    std::string out;
    out += "{\"entities\":[";
    const auto alive_entities = entity_manager_.aliveEntities();
    for (size_t i = 0; i < alive_entities.size(); ++i) {
        const auto& entity = alive_entities[i];
        if (i > 0) {
            out += ",";
        }
        out += "{\"index\":" + std::to_string(entity.index) + ",\"generation\":" + std::to_string(entity.generation) + "}";
    }

    out += "],\"components\":{";
    bool first_component = true;
    for (const auto& [name, serializer] : serializers_) {
        if (!first_component) {
            out += ",";
        }
        first_component = false;
        out += "\"" + name + "\":[" + serializer() + "]";
    }
    out += "}}";
    return out;
}

void World::deserializeWorld(const std::string& text) {
    storages_.clear();
    entity_manager_ = EntityManager();

    Parser parser(text);
    parser.expect('{');
    if (parser.parseString() != "entities") {
        throw std::runtime_error("Missing entities");
    }
    parser.expect(':');
    parser.expect('[');

    std::vector<EntityId> entities;
    while (!parser.consume(']')) {
        parser.expect('{');
        if (parser.parseString() != "index") {
            throw std::runtime_error("Expected index");
        }
        parser.expect(':');
        uint32_t index = parser.parseUInt();
        parser.expect(',');
        if (parser.parseString() != "generation") {
            throw std::runtime_error("Expected generation");
        }
        parser.expect(':');
        uint32_t generation = parser.parseUInt();
        parser.expect('}');
        entities.push_back(EntityId{index, generation});
        parser.consume(',');
    }

    std::sort(entities.begin(), entities.end(), [](const EntityId& a, const EntityId& b) { return a.index < b.index; });

    std::vector<EntityId> remap;
    for (const EntityId old_entity : entities) {
        while (remap.size() <= old_entity.index) {
            remap.push_back(createEntity());
        }

        EntityId current = remap[old_entity.index];
        while (current.generation < old_entity.generation) {
            destroyEntity(current);
            current = createEntity();
        }
        remap[old_entity.index] = current;
    }

    parser.expect(',');
    if (parser.parseString() != "components") {
        throw std::runtime_error("Expected components");
    }
    parser.expect(':');
    parser.expect('{');

    while (!parser.consume('}')) {
        std::string component_name = parser.parseString();
        parser.expect(':');
        parser.expect('[');

        auto deserializer = deserializers_.find(component_name);
        while (!parser.consume(']')) {
            parser.expect('{');
            if (parser.parseString() != "entity") {
                throw std::runtime_error("Expected entity key");
            }
            parser.expect(':');
            parser.expect('{');
            if (parser.parseString() != "index") {
                throw std::runtime_error("Expected entity index");
            }
            parser.expect(':');
            uint32_t index = parser.parseUInt();
            parser.expect(',');
            if (parser.parseString() != "generation") {
                throw std::runtime_error("Expected entity generation");
            }
            parser.expect(':');
            (void)parser.parseUInt();
            parser.expect('}');
            parser.expect(',');
            if (parser.parseString() != "data") {
                throw std::runtime_error("Expected data key");
            }
            parser.expect(':');
            std::string payload = parser.parseString();
            parser.expect('}');

            if (deserializer != deserializers_.end()) {
                deserializer->second(remap.at(index), payload);
            }
            parser.consume(',');
        }

        parser.consume(',');
    }

    parser.expect('}');
}

}  // namespace ecs
