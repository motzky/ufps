#pragma once

#include <optional>

#include "graphics/mesh_data.h"
#include "graphics/texture.h"

namespace ufps
{
    struct ModelData
    {
        MeshData mesh_data;
        std::optional<TextureData> albedo;
        std::optional<TextureData> normal;
        std::optional<TextureData> specular;
        std::optional<TextureData> roughness;
        std::optional<TextureData> ambient_occlusion;
        std::optional<TextureData> emissive_color;
    };

}