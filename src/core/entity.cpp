#include "core/entity.h"

#include <ranges>

#include "core/entity_manager.h"
#include "core/service_locator.h"
#include "math/transform.h"

namespace ufps
{
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