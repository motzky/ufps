#include "core/entity.h"

#include <ranges>

#include "core/entity_manager.h"
#include "core/service_locator.h"
#include "math/transform.h"

namespace ufps
{
    auto Entity::description() const -> Description
    {
        const auto &[em, rem, ps] = services<EntityManager, RenderEntityManager, PhysicsSystem>();

        auto render_entities = _render_entities |
                               std::views::filter([&](auto e)
                                                  { return !!rem[e]; }) |
                               std::views::transform([&](auto e)
                                                     { return std::string{rem[e]->group_name()}; }) |
                               std::ranges::to<std::vector>();

        std::ranges::sort(render_entities);
        const auto [first, last] = std::ranges::unique(render_entities);
        render_entities.erase(first, last);

        return {
            .name = _name,
            .emissive_strength = _emissive_strength,
            .transform = _transform,
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
        };
    }

    auto Entity::set_transform(const Transform &transform) -> void
    {
        auto &&[mgr, ps] = services<EntityManager, PhysicsSystem>();

        update_transforms(transform, _parent_transform);

        for (auto handle : _children)
        {
            const auto child = mgr[handle];
            child->set_parent_transform(transform);
        }

        _rigid_bodies = _rigid_bodies |
                        std::views::filter([&ps](auto e)
                                           { return !!ps.rigid_body(e); }) |
                        std::ranges::to<std::vector>();

        for (const auto handle : _rigid_bodies)
        {
            ps.rigid_body(handle)->set_parent_transform(_transform);
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

}