#pragma once

#include <ranges>

#include "core/entity.h"
#include "core/sparse_set.h"
#include "utils/ensure.h"
#include "utils/string_unordered_map.h"

namespace ufps
{
    using EntityHandle = SparseSet<Entity>::handle_type;

    class EntityManager
    {
    public:
        constexpr auto register_entity(std::string_view name, Entity entities) -> EntityHandle;

        constexpr auto operator[](std::string_view name) -> EntityHandle;
        constexpr auto operator[](EntityHandle handle);

    private:
        SparseSet<Entity> _entities;
        StringUnorderedMap<EntityHandle> _entity_names;
    };

    constexpr auto EntityManager::register_entity(std::string_view name, Entity entity) -> EntityHandle
    {
        auto handle = _entities.emplace(std::move(entity));

        _entity_names[std::string{name}] = handle;
        return handle;
    }

    constexpr auto EntityManager::operator[](std::string_view name) -> EntityHandle
    {
        auto element = _entity_names.find(name);
        ensure(element != std::ranges::cend(_entity_names), "{} does not exist", name);

        return element->second;
    }

    constexpr auto EntityManager::operator[](EntityHandle handle)
    {
        return _entities[handle];
    }

}