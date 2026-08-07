#pragma once

#include <algorithm>
#include <span>

#include "core/render_entity.h"
#include "core/render_entity_manager.h"
#include "core/service_locator.h"
#include "math/aabb.h"
#include "math/transform.h"

namespace ufps
{
    constexpr auto create_aabb(std::span<ufps::RenderEntityHandle> render_entities) -> ufps::AABB
    {
        auto initial_aabb = ufps::AABB{
            .min = {std::numeric_limits<float>::max()},
            .max = {std::numeric_limits<float>::lowest()}};

        auto rem = service<RenderEntityManager>();

        return std::ranges::fold_left(
            render_entities,
            initial_aabb,
            [&rem](const auto &a, const auto &e)
            {
                auto entity = rem[e];

                return ufps::AABB{
                    .min = {
                        std::min(a.min.x, entity->aabb().min.x),
                        std::min(a.min.y, entity->aabb().min.y),
                        std::min(a.min.z, entity->aabb().min.z),
                    },
                    .max = {
                        std::max(a.max.x, entity->aabb().max.x),
                        std::max(a.max.y, entity->aabb().max.y),
                        std::max(a.max.z, entity->aabb().max.z),
                    }};
            });
    }
}
