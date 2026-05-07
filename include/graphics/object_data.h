#pragma once

#include <cstdint>

#include "math/matrix4.h"

namespace ufps
{
    struct alignas(16) ObjectData
    {
        Matrix4 model;
        std::uint64_t albedo_texture_bindless_handle;
        std::uint64_t normal_texture_bindless_handle;
        std::uint64_t specular_texture_bindless_handle;
        std::uint64_t roughness_texture_bindless_handle;
        std::uint64_t ao_texture_bindless_handle;
        std::uint64_t emissive_texture_bindless_handle;
        float opacity;
        std::uint32_t normal_compressed;
        std::uint32_t pad;
    };
}