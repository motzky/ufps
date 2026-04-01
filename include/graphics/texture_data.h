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
        SRGB,
        RGBA,
        SRGBA,
        RGB16F,
        DEPTH24,
        BC7,
    };

    struct TextureData
    {
        std::uint32_t width;
        std::uint32_t height;
        TextureFormat format;
        std::optional<DataBuffer> data;
        bool is_compressed;
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
        case SRGB:
            return "RGB";
        case RGBA:
            return "RGB";
        case SRGBA:
            return "SRGBA";
        case RGB16F:
            return "RGB16F";
        case DEPTH24:
            return "DEPTH24";
        case BC7:
            return "BC7";
        default:
            return "unknown";
        }
    }

}