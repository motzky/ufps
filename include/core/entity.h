#pragma once

#include <algorithm>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#include "core/render_entity.h"
#include "core/service_locator.h"
#include "core/utils.h"
#include "math/aabb.h"
#include "math/transform.h"
#include "physics/physics_system.h"

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
            std::vector<RigidBody::Description> rigid_bodies;
        };

        constexpr Entity(std::string name, std::vector<RenderEntity> render_entities, Transform transform);

        constexpr auto name() const -> std::string;
        constexpr auto render_entities() const -> std::span<const RenderEntity>;
        constexpr auto transform() const -> const Transform &;
        constexpr auto set_transform(const Transform &transform) -> void;
        constexpr auto aabb() const -> const AABB &;
        constexpr auto description() const -> Description;
        constexpr auto emissive_strength() const -> float;
        constexpr auto set_emissive_strength(float strength) -> void;
        constexpr auto add_rigid_body(RigidBodyHandle handle) -> void;
        constexpr auto rigid_bodies() const -> std::span<const RigidBodyHandle>;

    private:
        std::string _name;
        std::vector<RenderEntity> _render_entities;
        std::vector<RigidBodyHandle> _rigid_bodies;
        Transform _transform;
        AABB _aabb;
        float _emissive_strength;
    };

    constexpr Entity::Entity(std::string name, std::vector<RenderEntity> render_entities, Transform transform)
        : _name{std::move(name)},
          _render_entities{std::move(render_entities)},
          _rigid_bodies{},
          _transform{std::move(transform)},
          _aabb{create_aabb(_render_entities)},
          _emissive_strength{1.f}
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

        for (const auto handle : _rigid_bodies)
        {
            auto body = service<PhysicsSystem>().rigid_body(handle);
            if (body)
            {
                body->set_parent_transform(_transform);
            }
        }
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
            .rigid_bodies = _rigid_bodies |
                            std::views::transform(
                                [](auto e)
                                {
                                    auto &physics = service<PhysicsSystem>();
                                    return physics.rigid_body(e);
                                }) |
                            std::views::filter([](const auto &e)
                                               { return !!e; }) |
                            std::views::transform([](const auto &e)
                                                  { return e->description(); }) |
                            std::ranges::to<std::vector>()};
    }

    constexpr auto Entity::emissive_strength() const -> float
    {
        return _emissive_strength;
    }

    constexpr auto Entity::set_emissive_strength(float strength) -> void
    {
        _emissive_strength = strength;
    }

    constexpr auto Entity::add_rigid_body(RigidBodyHandle handle) -> void
    {
        _rigid_bodies.push_back(handle);
        auto body = service<PhysicsSystem>().rigid_body(handle);
        if (body)
        {
            body->set_parent_transform(_transform);
        }
    }

    constexpr auto Entity::rigid_bodies() const -> std::span<const RigidBodyHandle>
    {
        return _rigid_bodies;
    }
}