#include "core/entity.h"

#include <algorithm>
#include <span>
#include <string>
#include <vector>

#include "core/render_entity.h"
#include "math/aabb.h"
#include "math/transform.h"

namespace
{
    auto create_aabb(std::span<ufps::RenderEntity> render_entities) -> ufps::AABB
    {
        auto initial_aabb = ufps::AABB{
            .min = {std::numeric_limits<float>::max()},
            .max = {std::numeric_limits<float>::lowest()}};

        return std::ranges::fold_left(
            render_entities,
            initial_aabb,
            [](const auto &a, const auto &e)
            {
                return ufps::AABB{
                    .min = {
                        std::min(a.min.x, e.aabb().min.x),
                        std::min(a.min.y, e.aabb().min.y),
                        std::min(a.min.z, e.aabb().min.z),
                    },
                    .max = {
                        std::max(a.max.x, e.aabb().max.x),
                        std::max(a.max.y, e.aabb().max.y),
                        std::max(a.max.z, e.aabb().max.z),
                    }};
            });
    }
}

namespace ufps
{

    Entity::Entity(std::string name, std::vector<RenderEntity> render_entities, Transform transform)
        : _name{std::move(name)},
          _render_entities{std::move(render_entities)},
          _transform{std::move(transform)},
          _aabb{create_aabb(_render_entities)}
    {
    }

    auto Entity::name() const -> std::string
    {
        return _name;
    }

    auto Entity::render_entities() const -> std::span<const RenderEntity>
    {
        return _render_entities;
    }

    auto Entity::transform() const -> const Transform &
    {
        return _transform;
    }

    auto Entity::set_transform(const Transform &transform) -> void
    {
        _transform = transform;
    }

    auto Entity::aabb() const -> const AABB &
    {
        return _aabb;
    }

}