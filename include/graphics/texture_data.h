#pragma once

#include <cstdint>
#include <string>

#include "utils/data_buffer.h"

namespace ufps
{
    enum class TextureFormat
    {
        RGB,
        RGBA,
    };

    struct TextureData
    {
        std::uint32_t width;
        std::uint32_t height;
        TextureFormat format;
        DataBuffer data;
    };

    inline auto to_string(TextureFormat obj) -> std::string
    {
        switch (obj)
        {
            using enum TextureFormat;
        case RGB:
            return "RGB";
        case RGBA:
            return "RGBA";
        default:
            return "unknown";
        }
    }

}