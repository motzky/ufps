#pragma once

#include <ranges>
#include <span>
#include <vector>

#include "core/render_entity.h"
#include "core/sparse_set.h"
#include "utils/ensure.h"
#include "utils/string_unordered_map.h"

namespace ufps
{
    using RenderEntityHandle = SparseSet<RenderEntity>::handle_type;

    class RenderEntityManager
    {
    public:
        constexpr auto register_group(std::string_view name, std::vector<RenderEntity> entities) -> void;

        constexpr auto operator[](std::string_view name) -> std::vector<RenderEntityHandle>;
        constexpr auto operator[](RenderEntityHandle handle);

    private:
        SparseSet<RenderEntity> _entities;
        StringUnorderedMap<std::vector<RenderEntityHandle>> _entity_groups;
    };

    constexpr auto RenderEntityManager::register_group(std::string_view name, std::vector<RenderEntity> entities) -> void
    {
        auto handles = entities |
                       std::views::transform([this](auto &e)
                                             { return _entities.emplace(std::move(e)); }) |
                       std::ranges::to<std::vector>();

        _entity_groups[std::string{name}] = std::move(handles);
    }

    constexpr auto RenderEntityManager::operator[](std::string_view name) -> std::vector<RenderEntityHandle>
    {
        auto element = _entity_groups.find(name);
        ensure(element != std::ranges::cend(_entity_groups), "{} does not exist", name);

        return element->second;
    }

    constexpr auto RenderEntityManager::operator[](RenderEntityHandle handle)
    {
        return _entities[handle];
    }

}