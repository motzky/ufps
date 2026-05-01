#pragma once

#include <algorithm>
#include <span>
#include <string>
#include <vector>

#include "core/render_entity.h"
#include "core/utils.h"
#include "math/aabb.h"
#include "math/transform.h"

namespace ufps
{
    class Entity
    {
    public:
        struct Description
        {
            std::string name;
            Transform transform;
            AABB aabb;
        };

        constexpr Entity(std::string name, std::vector<RenderEntity> render_entities, Transform transform);

        constexpr auto name() const -> std::string;
        constexpr auto render_entities() const -> std::span<const RenderEntity>;
        constexpr auto transform() const -> const Transform &;
        constexpr auto set_transform(const Transform &transform) -> void;
        constexpr auto aabb() const -> const AABB &;
        constexpr auto description() const -> Description;

    private:
        std::string _name;
        std::vector<RenderEntity> _render_entities;
        Transform _transform;
        AABB _aabb;
    };

    constexpr Entity::Entity(std::string name, std::vector<RenderEntity> render_entities, Transform transform)
        : _name{std::move(name)},
          _render_entities{std::move(render_entities)},
          _transform{std::move(transform)},
          _aabb{create_aabb(_render_entities)}
    {
    }

    constexpr auto Entity::name() const -> std::string
    {
        return _name;
    }

    constexpr auto Entity::render_entities() const -> std::span<const RenderEntity>
    {
        return _render_entities;
    }

    constexpr auto Entity::transform() const -> const Transform &
    {
        return _transform;
    }

    constexpr auto Entity::set_transform(const Transform &transform) -> void
    {
        _transform = transform;
    }

    constexpr auto Entity::aabb() const -> const AABB &
    {
        return _aabb;
    }

    constexpr auto Entity::description() const -> Description
    {
        return {
            .name = _name,
            .transform = _transform,
            .aabb = _aabb,
        };
    }
}