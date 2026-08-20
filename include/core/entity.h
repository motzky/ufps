#pragma once

#include <algorithm>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#include "core/render_entity.h"
#include "core/render_entity_manager.h"
#include "core/service_locator.h"
#include "core/utils.h"
#include "math/aabb.h"
#include "math/transform.h"
#include "physics/physics_system.h"

namespace ufps
{
    class Entity
    {
        using EntityHandle = SparseSet<Entity>::handle_type;

    public:
        struct Description
        {
            std::string name;
            float emissive_strength;
            Transform transform;
            AABB aabb;
            std::vector<RigidBody::Description> rigid_bodies;
            std::vector<std::string> render_entities;
            std::vector<std::string> children;
        };

        constexpr Entity(std::string name, std::span<const RenderEntityHandle> render_entities, Transform transform);

        constexpr auto name() const -> std::string_view;
        constexpr auto set_name(std::string name) -> void;
        constexpr auto render_entities() const -> std::span<const RenderEntityHandle>;
        constexpr auto add_render_entities(std::span<const RenderEntityHandle> render_entities) -> void;
        constexpr auto remove_render_entity(RenderEntityHandle handle) //
            pre(std::ranges::contains(_render_entities, handle));
        constexpr auto transform() const -> const Transform &;
        constexpr auto parent_transform() const -> const Transform &;
        auto set_transform(const Transform &transform) -> void;
        constexpr auto aabb() const -> const AABB &;
        auto description() const -> Description;
        constexpr auto emissive_strength() const -> float;
        constexpr auto set_emissive_strength(float strength) -> void;
        constexpr auto add_rigid_body(RigidBodyHandle handle) -> void;
        constexpr auto rigid_bodies() const -> std::span<const RigidBodyHandle>;
        auto add_child(EntityHandle child) -> void;
        constexpr auto children() -> std::span<const EntityHandle>;

    private:
        auto update_transforms(const Transform &local, const Transform &parent) -> void;
        constexpr auto set_parent_transform(const Transform &transform) -> void;

        std::string _name;
        std::vector<RenderEntityHandle> _render_entities;
        std::vector<RigidBodyHandle> _rigid_bodies;
        Transform _local_transform;
        Transform _parent_transform;
        Transform _transform;
        AABB _aabb;
        float _emissive_strength;
        std::vector<EntityHandle> _children;
    };

    constexpr Entity::Entity(std::string name, std::span<const RenderEntityHandle> render_entities, Transform transform)
        : _name{std::move(name)},
          _render_entities{std::ranges::cbegin(render_entities), std::ranges::cend(render_entities)},
          _rigid_bodies{},
          _local_transform{std::move(transform)},
          _parent_transform{{}, {1.f}, {}},
          _transform{_parent_transform * _local_transform},
          _aabb{create_aabb(_render_entities)},
          _emissive_strength{1.f}
    {
    }

    constexpr auto Entity::name() const -> std::string_view
    {
        return _name;
    }

    constexpr auto Entity::set_name(std::string name) -> void
    {
        _name = std::move(name);
    }

    constexpr auto Entity::render_entities() const -> std::span<const RenderEntityHandle>
    {
        return _render_entities;
    }

    constexpr auto Entity::add_render_entities(std::span<const RenderEntityHandle> render_entities) -> void
    {
        _render_entities.append_range(render_entities);
        _aabb = create_aabb(_render_entities);
    }

    constexpr auto Entity::remove_render_entity(RenderEntityHandle handle)
    {
        std::erase(_render_entities, handle);
    }

    constexpr auto Entity::transform() const -> const Transform &
    {
        return _transform;
    }

    constexpr auto Entity::parent_transform() const -> const Transform &
    {
        return _parent_transform;
    }

    constexpr auto Entity::aabb() const -> const AABB &
    {
        return _aabb;
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

    constexpr auto Entity::set_parent_transform(const Transform &transform) -> void
    {
        update_transforms(_local_transform, transform);
    }

    constexpr auto Entity::children() -> std::span<const EntityHandle>
    {
        return _children;
    }
}