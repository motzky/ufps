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

    struct Scene
    {
        constexpr auto intersect_ray(const Ray &ray) const -> std::optional<IntersectionResult>
        {
            auto result = std::optional<IntersectionResult>{};
            auto min_distance = std::numeric_limits<float>::max();

            for (const auto &entity : entities)
            {
                const auto inv_transform = Matrix4::invert(entity.transform);
                const auto transformed_ray =
                    Ray{inv_transform * Vector4{ray.origin, 1.0f}, inv_transform * Vector4{ray.direction, 0.0f}};

                const auto index_data = mesh_manager.index_data(entity.mesh_view);
                const auto vertex_data = mesh_manager.vertex_data(entity.mesh_view);

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

            return result;
        }

        std::vector<Entity> entities;
        MeshManager &mesh_manager;
        MaterialManager &material_manager;
        TextureManager &texture_manager;
        Camera camera;
        LightData lights;
    };
}