#pragma once

#include <deque>
#include <optional>
#include <ranges>
#include <vector>

#include "core/camera_manager.h"
#include "core/entity.h"
#include "core/entity_manager.h"
#include "core/light_manager.h"
#include "core/render_entity_manager.h"
#include "core/service_locator.h"
#include "core/sparse_set.h"
#include "graphics/color.h"
#include "graphics/point_light.h"
#include "math/bounded_number.h"
#include "math/ray.h"
#include "math/utils.h"
#include "math/vector4.h"

namespace ufps
{
    struct IntersectionResult
    {
        EntityHandle entity;
        Vector3 position;
        float distance;
    };

    struct ToneMapOptions
    {
        BoundedFloat<0.f, 100.f> max_brightness = 1.f;
        BoundedFloat<0.f, 50.f> contrast = 1.f;
        BoundedFloat<0.f, 1.f> linear_section_start = .22f;
        BoundedFloat<0.f, 1.f> linear_section_length = .4f;
        BoundedFloat<0.f, 3.f> black_tightness = 1.33f;
        BoundedFloat<0.f, 1.f> pedestal = 0.f;
        BoundedFloat<0.f, 5.f> gamma = 2.2f;
    };

    struct SSAOOptions
    {
        bool enabled = true;
        BoundedUint32<1u, 64u> sample_count = 64u;
        BoundedFloat<0.1f, 2.f> radius = .75f;
        BoundedFloat<0.01f, 0.1f> bias = .025f;
        BoundedFloat<1.f, 4.f> power = 2.f;
    };

    struct ExposureOptions
    {
        BoundedFloat<-10.f, 10.f> min_log_luminance = -8.f;
        BoundedFloat<-10.f, 10.f> max_log_luminance = 3.5f;
        BoundedFloat<.1f, 5.f> tau = 3.0f;
    };

    struct FogOptions
    {
        Color color = Color::black();
        BoundedFloat<0.f, 0.2f> density = 0.02f;
    };

    struct ChromaticAbberationOptions
    {
        BoundedFloat<-0.1f, 0.1f> red_offset = .009f;
        BoundedFloat<-0.1f, 0.1f> green_offset = .006f;
        BoundedFloat<-0.1f, 0.1f> blue_offset = -.006f;
        BoundedFloat<0.0f, 1.f> strength = .5f;
    };

    struct VignetteOptions
    {
        Color color = Color::black();
        BoundedFloat<0.f, 1.f> strength = .5f;
        BoundedFloat<0.f, 1.f> feather = .1f;
    };

    struct FilmGrainOptions
    {
        BoundedFloat<0.f, 1.f> strength = .01f;
    };

    struct BloomOptions
    {
        BoundedFloat<0.f, .1f> filter_radius = .005f;
        BoundedFloat<0.f, 1.f> mix_amount = .04f;
        BoundedFloat<0.f, 10.f> threshold = 1.f;
    };

    class Scene
    {
    public:
        struct Description
        {
            ToneMapOptions tone_map_options;
            SSAOOptions ssao_options;
            ExposureOptions exposure_options;
            FogOptions fog_options;
            ChromaticAbberationOptions chromatic_abberation_options;
            VignetteOptions vignette_options;
            FilmGrainOptions film_grain_options;
            BloomOptions bloom_options;
            Color ambient;
            std::vector<PointLight> lights;
            std::vector<Entity::Description> entities;
        };

        constexpr Scene(const Description &description);

        constexpr auto intersect_ray(const Ray &ray) -> std::optional<IntersectionResult>;

        constexpr auto add(EntityHandle handle) -> void;

        constexpr auto entities() const -> std::span<const EntityHandle>;

        constexpr auto gun() const -> EntityHandle;

        constexpr auto &ambient_light(this auto &&self);

        constexpr auto &tone_map_options(this auto &&self);
        constexpr auto &ssao_options(this auto &&self);
        constexpr auto &exposure_options(this auto &&self);
        constexpr auto &fog_options(this auto &&self);
        constexpr auto &chromatic_abberation_options(this auto &&self);
        constexpr auto &vignette_options(this auto &&self);
        constexpr auto &film_grain_options(this auto &&self);
        constexpr auto &bloom_options(this auto &&self);

        constexpr auto description(this auto &&self) -> Description;

        constexpr auto remove(EntityHandle entity) -> void;

    private:
        std::vector<EntityHandle> _entities;
        EntityHandle _gun;
        Color _ambient;
        ToneMapOptions _tone_map_options;
        SSAOOptions _ssao_options;
        ExposureOptions _exposure_options;
        FogOptions _fog_options;
        ChromaticAbberationOptions _chromatic_abberation_options;
        VignetteOptions _vignette_options;
        FilmGrainOptions _film_grain_options;
        BloomOptions _bloom_options;
    };

    constexpr auto Scene::intersect_ray(const Ray &ray) -> std::optional<IntersectionResult>
    {
        auto &&[mesh_manager, rem, em] = ufps::services<MeshManager, RenderEntityManager, EntityManager>();

        auto result = std::optional<IntersectionResult>{};
        auto min_distance = std::numeric_limits<float>::max();

        for (auto handle : _entities)
        {
            auto entity = em[handle];
            if (!entity)
            {
                continue;
            }

            const auto inv_transform = Matrix4::invert(entity->transform());
            const auto transformed_ray =
                Ray{inv_transform * Vector4{ray.origin, 1.0f}, inv_transform * Vector4{ray.direction, 0.0f}};

            if (!!intersect(transformed_ray, entity->aabb()))
            {
                for (auto render_entity_handle : entity->render_entities())
                {
                    if (auto render_entity = rem[render_entity_handle]; render_entity)
                    {
                        if (!intersect(transformed_ray, render_entity->aabb()))
                        {
                            continue;
                        }

                        const auto mesh_view = render_entity->mesh_view();
                        const auto index_data = mesh_manager.index_data(mesh_view);
                        const auto vertex_data = mesh_manager.vertex_data(mesh_view);

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
                                    result = IntersectionResult{.entity = handle, .position = intersection_point, .distance = *distance};
                                    min_distance = *distance;
                                }
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    constexpr Scene::Scene(const Description &description)
        : _entities{},
          _gun{},
          _ambient{description.ambient},
          _tone_map_options{description.tone_map_options},
          _ssao_options{description.ssao_options},
          _exposure_options{description.exposure_options},
          _fog_options{description.fog_options},
          _chromatic_abberation_options{description.chromatic_abberation_options},
          _vignette_options{description.vignette_options},
          _film_grain_options{description.film_grain_options},
          _bloom_options{description.bloom_options}
    {
        auto &&[em, rem, ps, lm, cm] = services<EntityManager, RenderEntityManager, PhysicsSystem, LightManager, CameraManager>();

        for (auto &light : description.lights)
        {
            lm.insert(light);
        }

        auto lookup = StringUnorderedMap<EntityHandle>{};

        for (const auto &entity_description : description.entities)
        {
            const auto new_entity_handle = em.insert({entity_description.name, {}, entity_description.transform});

            auto new_entity = em[new_entity_handle];
            new_entity->set_transform(entity_description.transform);

            new_entity->set_emissive_strength(entity_description.emissive_strength);

            for (const auto &group_name : entity_description.render_entities)
            {
                new_entity->add_render_entities(rem[group_name]);
            }

            for (const auto &rb_desc : entity_description.rigid_bodies)
            {
                const auto rb = ps.create_rigid_body(rb_desc);
                new_entity->add_rigid_body(rb);
            }

            if (entity_description.camera)
            {
                const auto camera_handle = cm.insert(*entity_description.camera);
                new_entity->set_camera(camera_handle);
            }

            if (new_entity->name() == "gun")
            {
                _gun = new_entity_handle;
            }
            else
            {
                add(new_entity_handle);
            }
            lookup[entity_description.name] = new_entity_handle;
        }

        for (const auto &entity_description : description.entities)
        {
            const auto &entity = lookup[entity_description.name];

            for (const auto &child : entity_description.children)
            {
                const auto child_handle = lookup.find(child);
                ensure(child_handle != std::ranges::cend(lookup), "child {} not found", child);

                em[entity]->add_child(child_handle->second);
            }
        }
    }

    constexpr auto Scene::add(EntityHandle handle) -> void
    {
        _entities.push_back(handle);
    }

    constexpr auto Scene::entities() const -> std::span<const EntityHandle>
    {
        return _entities;
    }

    constexpr auto Scene::gun() const -> EntityHandle
    {
        return _gun;
    }

    constexpr auto &Scene::ambient_light(this auto &&self)
    {
        return self._ambient;
    }

    constexpr auto &Scene::tone_map_options(this auto &&self)
    {
        return self._tone_map_options;
    }

    constexpr auto &Scene::ssao_options(this auto &&self)
    {
        return self._ssao_options;
    }

    constexpr auto &Scene::exposure_options(this auto &&self)
    {
        return self._exposure_options;
    }

    constexpr auto &Scene::fog_options(this auto &&self)
    {
        return self._fog_options;
    }

    constexpr auto &Scene::chromatic_abberation_options(this auto &&self)
    {
        return self._chromatic_abberation_options;
    }

    constexpr auto &Scene::vignette_options(this auto &&self)
    {
        return self._vignette_options;
    }

    constexpr auto &Scene::film_grain_options(this auto &&self)
    {
        return self._film_grain_options;
    }

    constexpr auto &Scene::bloom_options(this auto &&self)
    {
        return self._bloom_options;
    }

    constexpr auto Scene::description(this auto &&self) -> Description
    {
        const auto &[em, lm] = services<EntityManager, LightManager>();

        return Description{
            .tone_map_options = self._tone_map_options,
            .ssao_options = self._ssao_options,
            .exposure_options = self._exposure_options,
            .fog_options = self._fog_options,
            .chromatic_abberation_options = self._chromatic_abberation_options,
            .vignette_options = self._vignette_options,
            .film_grain_options = self._film_grain_options,
            .bloom_options = self._bloom_options,
            .ambient = self._ambient,
            .lights = lm.data() |
                      std::ranges::to<std::vector>(),
            .entities = self._entities |
                        std::views::filter([&](auto e)
                                           { return !!em[e]; }) |
                        std::views::transform([&](auto e)
                                              { return em[e]->description(); }) |
                        std::ranges::to<std::vector>()};
    }

    constexpr auto Scene::remove(EntityHandle handle) -> void
    {
        const auto iter = std::ranges::find(_entities, handle);
        expect(iter != std::ranges::cend(_entities), "Entity not found");

        _entities.erase(iter);
    }

}