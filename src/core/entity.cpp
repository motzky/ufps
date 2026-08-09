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

}