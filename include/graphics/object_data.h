#pragma once

#include <cstdint>

#include "math/matrix4.h"

namespace ufps
{
    struct alignas(16) ObjectData
    {
        Matrix4 model;
        std::uint32_t albedo_texture_index;
        std::uint32_t normal_texture_index;
        std::uint32_t specular_texture_index;
        std::uint32_t roughness_texture_index;
        std::uint32_t ao_texture_index;
        std::uint32_t emissive_texture_index;
        std::uint32_t normal_compressed;
        std::uint32_t padding;
        float opacity;
    };

}