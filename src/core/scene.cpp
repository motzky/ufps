#include "core/scene.h"

#include <ranges>
#include <span>
#include <string_view>

#include "core/camera.h"
#include "core/entity.h"
#include "utils/ensure.h"

namespace ufps
{
    Scene::Scene(MeshManager &mesh_manager, MaterialManager &material_manager, TextureManager &texture_manager, Camera camera, LightData lights,
                 ToneMapOptions tone_map_options, SSAOOptions ssao_options, ExposureOptions exposure_options)
        : _entities{},
          _entity_cache{},
          _mesh_manager{mesh_manager},
          _material_manager{material_manager},
          _texture_manager{texture_manager},
          _camera{std::move(camera)},
          _lights{std::move(lights)},
          _tone_map_options{std::move(tone_map_options)},
          _ssao_options{std::move(ssao_options)},
          _exposure_options{std::move(exposure_options)}
    {
    }

    auto Scene::create_entity(std::string_view name) -> Entity *
    {
        const auto cached = std::ranges::find_if(_entity_cache, [name](const auto &e)
                                                 { return e.name() == name; });
        expect(cached != std::ranges::cend(_entity_cache), "unknown entity: {}", name);

        auto &new_entity = _entities.emplace_back(*cached);
        new_entity.set_transform({});

        return &new_entity;
    }

    auto Scene::cache_entity(std::string_view name, Entity entity) -> void
    {
        const auto cached = std::ranges::find_if(_entity_cache, [name](const auto &e)
                                                 { return e.name() == name; });
        expect(cached == std::ranges::cend(_entity_cache), "entity already exists: {}", name);

        _entity_cache.push_back(std::move(entity));
    }
}