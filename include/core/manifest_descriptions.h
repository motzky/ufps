#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "graphics/mesh_view.h"
#include "utils/string_unordered_map.h"

namespace ufps
{
    struct ModelManifest
    {
        MeshView mesh_view;
        std::string albedo_texture;
        std::string normal_texture;
        std::string specular_texture;
        std::string roughness_texture;
        std::string ambient_occlusion_texture;
        std::string emissive_color_texture;
        bool normal_compressed;
    };

    struct ModelManifestDescription
    {
        StringUnorderedMap<std::vector<ModelManifest>> models;
    };

    struct TextureManifest
    {
        std::uint32_t offset;
        std::uint32_t size;
        bool is_srgb;
    };

    struct TextureManifestDescription
    {
        StringUnorderedMap<TextureManifest> textures;
    };
}