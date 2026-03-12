#pragma once

#include <span>
#include <string>
#include <vector>

#include "core/render_entity.h"
#include "math/aabb.h"
#include "math/transform.h"

namespace ufps
{
    class Entity
    {
    public:
        Entity(std::string name, std::vector<RenderEntity> render_entities, Transform transform);

        auto name() const -> std::string;
        auto render_entities() const -> std::span<const RenderEntity>;
        auto transform() const -> const Transform &;
        auto set_transform(const Transform &transform) -> void;
        auto aabb() const -> const AABB &;

    private:
        std::string _name;
        std::vector<RenderEntity> _render_entities;
        Transform _transform;
        AABB _aabb;
    };
}