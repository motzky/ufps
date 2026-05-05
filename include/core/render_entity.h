#pragma once

#include <algorithm>
#include <cstdint>

#include "graphics/mesh_manager.h"
#include "graphics/mesh_view.h"
#include "math/aabb.h"

namespace ufps
{
    namespace impl
    {
        constexpr auto calculate_aabb(ufps::MeshView mesh_view, const ufps::MeshManager &mesh_manager) -> ufps::AABB
        {
            const auto vertices = mesh_manager.vertex_data(mesh_view);

            auto initial_aabb = ufps::AABB{
                .min = {std::numeric_limits<float>::max()},
                .max = {std::numeric_limits<float>::lowest()}};

            return std::ranges::fold_left(
                vertices,
                initial_aabb,
                [](const auto &a, const auto &b)
                {
                    return ufps::AABB{
                        .min = {
                            std::min(a.min.x, b.position.x),
                            std::min(a.min.y, b.position.y),
                            std::min(a.min.z, b.position.z),
                        },
                        .max = {
                            std::max(a.max.x, b.position.x),
                            std::max(a.max.y, b.position.y),
                            std::max(a.max.z, b.position.z),
                        }};
                });
        }
    }

    class RenderEntity
    {
    public:
        constexpr RenderEntity(
            MeshView mesh_view,
            std::uint32_t albedo_texture_index,
            std::uint32_t normal_texture_index,
            std::uint32_t specular_texture_index,
            std::uint32_t roughness_texture_index,
            std::uint32_t ao_texture_index,
            std::uint32_t emissive_texture_index,
            bool normal_compressed,
            const MeshManager &mesh_manager);

        constexpr auto mesh_view() const -> MeshView;
        constexpr auto albedo_texture_index() const -> std::uint32_t;
        constexpr auto normal_texture_index() const -> std::uint32_t;
        constexpr auto specular_texture_index() const -> std::uint32_t;
        constexpr auto roughness_texture_index() const -> std::uint32_t;
        constexpr auto ao_texture_index() const -> std::uint32_t;
        constexpr auto emissive_texture_index() const -> std::uint32_t;
        constexpr auto normal_compressed() const -> bool;
        constexpr auto aabb() const -> const AABB &;

    private:
        MeshView _mesh_view;
        std::uint32_t _albedo_texture_index;
        std::uint32_t _normal_texture_index;
        std::uint32_t _specular_texture_index;
        std::uint32_t _roughness_texture_index;
        std::uint32_t _ao_texture_index;
        std::uint32_t _emissive_texture_index;
        bool _normal_compressed;
        AABB _aabb;
    };

    constexpr RenderEntity::RenderEntity(
        MeshView mesh_view,
        std::uint32_t albedo_texture_index,
        std::uint32_t normal_texture_index,
        std::uint32_t specular_texture_index,
        std::uint32_t roughness_texture_index,
        std::uint32_t ao_texture_index,
        std::uint32_t emissive_texture_index,
        bool normal_compressed,
        const MeshManager &mesh_manager)
        : _mesh_view{mesh_view},
          _albedo_texture_index{albedo_texture_index},
          _normal_texture_index{normal_texture_index},
          _specular_texture_index{specular_texture_index},
          _roughness_texture_index{roughness_texture_index},
          _ao_texture_index{ao_texture_index},
          _emissive_texture_index{emissive_texture_index},
          _normal_compressed{normal_compressed},
          _aabb{impl::calculate_aabb(mesh_view, mesh_manager)}
    {
    }

    constexpr auto RenderEntity::mesh_view() const -> MeshView
    {
        return _mesh_view;
    }

    constexpr auto RenderEntity::albedo_texture_index() const -> std::uint32_t
    {
        return _albedo_texture_index;
    }

    constexpr auto RenderEntity::normal_texture_index() const -> std::uint32_t
    {
        return _normal_texture_index;
    }

    constexpr auto RenderEntity::specular_texture_index() const -> std::uint32_t
    {
        return _specular_texture_index;
    }

    constexpr auto RenderEntity::roughness_texture_index() const -> std::uint32_t
    {
        return _roughness_texture_index;
    }

    constexpr auto RenderEntity::ao_texture_index() const -> std::uint32_t
    {
        return _ao_texture_index;
    }

    constexpr auto RenderEntity::emissive_texture_index() const -> std::uint32_t
    {
        return _emissive_texture_index;
    }

    constexpr auto RenderEntity::normal_compressed() const -> bool
    {
        return _normal_compressed;
    }

    constexpr auto RenderEntity::aabb() const -> const AABB &
    {
        return _aabb;
    }

}