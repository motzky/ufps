#include "core/entity.h"

#include <ranges>

#include "core/camera_manager.h"
#include "core/entity_manager.h"
#include "core/service_locator.h"
#include "math/transform.h"

namespace ufps
{
    Entity::Entity(std::string name, std::span<const RenderEntityHandle> render_entities, Transform transform)
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

    auto Entity::name() const -> std::string_view
    {
        return _name;
    }

    auto Entity::set_name(std::string name) -> void
    {
        _name = std::move(name);
    }

    auto Entity::render_entities() const -> std::span<const RenderEntityHandle>
    {
        return _render_entities;
    }

    auto Entity::add_render_entities(std::span<const RenderEntityHandle> render_entities) -> void
    {
        _render_entities.append_range(render_entities);
        _aabb = create_aabb(_render_entities);
    }

    auto Entity::remove_render_entity(RenderEntityHandle handle) -> void
    {
        std::erase(_render_entities, handle);
    }

    auto Entity::transform() const -> const Transform &
    {
        return _transform;
    }

    auto Entity::local_transform() const -> const Transform &
    {
        return _local_transform;
    }

    auto Entity::parent_transform() const -> const Transform &
    {
        return _parent_transform;
    }

    auto Entity::aabb() const -> const AABB &
    {
        return _aabb;
    }

    auto Entity::emissive_strength() const -> float
    {
        return _emissive_strength;
    }

    auto Entity::set_emissive_strength(float strength) -> void
    {
        _emissive_strength = strength;
    }

    auto Entity::add_rigid_body(RigidBodyHandle handle) -> void
    {
        _rigid_bodies.push_back(handle);
        auto body = service<PhysicsSystem>().rigid_body(handle);
        if (body)
        {
            body->set_parent_transform(_transform);
        }
    }

    auto Entity::rigid_bodies() const -> std::span<const RigidBodyHandle>
    {
        return _rigid_bodies;
    }

    auto Entity::set_parent_transform(const Transform &transform) -> void
    {
        update_transforms(_local_transform, transform);
    }

    auto Entity::children() -> std::span<const EntityHandle>
    {
        return _children;
    }

    auto Entity::description() const -> Description
    {
        const auto &[em, rem, ps, cm] = services<EntityManager, RenderEntityManager, PhysicsSystem, CameraManager>();

        auto render_entities = _render_entities |
                               std::views::filter([&](auto e)
                                                  { return !!rem[e]; }) |
                               std::views::transform([&](auto e)
                                                     { return std::string{rem[e]->group_name()}; }) |
                               std::ranges::to<std::vector>();

        std::ranges::sort(render_entities);
        const auto [first, last] = std::ranges::unique(render_entities);
        render_entities.erase(first, last);

        const auto camera = cm[_camera];

        return {
            .name = _name,
            .emissive_strength = _emissive_strength,
            .transform = _local_transform,
            .aabb = _aabb,
            .rigid_bodies = _rigid_bodies |
                            std::views::transform([&](auto e)
                                                  { return ps.rigid_body(e); }) |
                            std::views::filter([](const auto &e)
                                               { return !!e; }) |
                            std::views::transform([](const auto &e)
                                                  { return e->description(); }) |
                            std::ranges::to<std::vector>(),
            .render_entities = std::move(render_entities),
            .children = _children |
                        std::views::transform([&](auto e)
                                              { return em[e]; }) |
                        std::views::filter([](const auto &e)
                                           { return !!e; }) |
                        std::views::transform([](const auto &e)
                                              { return std::string{e->name()}; }) |
                        std::ranges::to<std::vector>(),
            .camera = camera.transform([](const auto &e)
                                       { return e.description(); })};
    }

    auto Entity::set_transform(const Transform &transform) -> void
    {
        auto &&[em, ps, cm] = services<EntityManager, PhysicsSystem, CameraManager>();

        update_transforms(transform, _parent_transform);

        for (auto handle : _children)
        {
            const auto child = em[handle];
            child->set_parent_transform(_transform);
        }

        _rigid_bodies = _rigid_bodies |
                        std::views::filter([&ps](auto e)
                                           { return !!ps.rigid_body(e); }) |
                        std::ranges::to<std::vector>();

        for (const auto handle : _rigid_bodies)
        {
            ps.rigid_body(handle)->set_parent_transform(_transform);
        }

        if (_camera)
        {
            const auto camera = cm[_camera];
            contract_assert(camera);

            camera->set_parent_transform(_transform);
        }
    }

    auto Entity::update_transforms(const Transform &local, const Transform &parent) -> void
    {
        _transform = parent * local;
        _local_transform = local;
        _parent_transform = parent;

        auto &&[em, ps] = services<EntityManager, PhysicsSystem>();

        for (auto handle : _children)
        {
            const auto child = em[handle];
            child->set_parent_transform(_transform);
        }
    }

    auto Entity::add_child(EntityHandle child) -> void
    {
        auto &em = service<EntityManager>();

        const auto entity = em[child];
        contract_assert(entity);

        entity->set_parent_transform(_transform);

        _children.push_back(child);
    }

    auto Entity::camera() const -> CameraHandle
    {
        return _camera;
    }

    auto Entity::set_camera(CameraHandle handle) -> void
    {
        _camera = handle;
    }

}