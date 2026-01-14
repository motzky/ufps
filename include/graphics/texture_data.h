#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "utils/data_buffer.h"

namespace ufps
{
    enum class TextureFormat
    {
        R,
        RGB,
        RGBA,
        RGB16F,
        DEPTH24
    };

    struct TextureData
    {
        std::uint32_t width;
        std::uint32_t height;
        TextureFormat format;
        std::optional<DataBuffer> data;
    };

    inline auto to_string(TextureFormat obj) -> std::string
    {
        switch (obj)
        {
            using enum TextureFormat;
        case R:
            return "R";
        case RGB:
            return "RGB";
        case RGBA:
            return "RGBA";
        case RGB16F:
            return "RGB16F";
        case DEPTH24:
            return "DEPTH24";
        default:
            return "unknown";
        }
    }

}