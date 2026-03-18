#pragma once

#include <optional>
#include <ranges>
#include <vector>

#include "core/camera.h"
#include "core/entity.h"
#include "graphics/color.h"
#include "graphics/material_manager.h"
#include "graphics/mesh_manager.h"
#include "graphics/point_light.h"
#include "graphics/texture_manager.h"
#include "math/ray.h"
#include "math/utils.h"
#include "math/vector4.h"

namespace ufps
{
    struct IntersectionResult
    {
        const Entity *entity;
        Vector3 position;
    };

    struct LightData
    {
        Color ambient;
        PointLight light;
    };

    class Scene
    {
    public:
        Scene(MeshManager &mesh_manager, MaterialManager &material_manager, TextureManager &texture_manager, Camera camera, LightData lights);

        constexpr auto intersect_ray(const Ray &ray) const -> std::optional<IntersectionResult>;

        auto create_entity(std::string_view name) -> void;

        template <class Self>
        auto entities(this Self &&self)
        {
            using SpanType = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const Entity, Entity>;
            return std::span<SpanType>{self._entities.data(), self._entities.data() + self._entities.size()};
        }

        auto cache_entity(std::string_view name, Entity entity) -> void;

        constexpr auto &camera(this auto &&self)
        {
            return self._camera;
        }

        constexpr auto &lights(this auto &&self)
        {
            return self._lights;
        }

        constexpr auto &mesh_manager(this auto &&self)
        {
            return self._mesh_manager;
        }

        constexpr auto &material_manager(this auto &&self)
        {
            return self._material_manager;
        }

        constexpr auto &texture_manager(this auto &&self)
        {
            return self._texture_manager;
        }

    private:
        std::vector<Entity> _entities;
        std::vector<Entity> _entity_cache;
        MeshManager &_mesh_manager;
        MaterialManager &_material_manager;
        TextureManager &_texture_manager;
        Camera _camera;
        LightData _lights;
    };

    constexpr auto Scene::intersect_ray(const Ray &ray) const -> std::optional<IntersectionResult>
    {
        auto result = std::optional<IntersectionResult>{};
        auto min_distance = std::numeric_limits<float>::max();

        for (const auto &entity : _entities)
        {
            const auto inv_transform = Matrix4::invert(entity.transform());
            const auto transformed_ray =
                Ray{inv_transform * Vector4{ray.origin, 1.0f}, inv_transform * Vector4{ray.direction, 0.0f}};

            for (const auto &render_entity : entity.render_entities())
            {
                const auto mesh_view = render_entity.mesh_view();
                const auto index_data = _mesh_manager.index_data(mesh_view);
                const auto vertex_data = _mesh_manager.vertex_data(mesh_view);

                for (const auto &indices : std::views::chunk(index_data, 3))
                {
                    const auto v0 = vertex_data[indices[0]].position;
                    const auto v1 = vertex_data[indices[1]].position;
                    const auto v2 = vertex_data[indices[2]].position;

                    if (const auto distance = intersect(transformed_ray, v0, v1, v2); distance)
                    {
                        const auto intersection_point = transformed_ray.origin + transformed_ray.direction * (*distance);

                        if (*distance < min_distance)
                        {
                            result = IntersectionResult{.entity = &entity, .position = intersection_point};
                            min_distance = *distance;
                        }
                    }
                }
            }
        }

        return result;
    }

}