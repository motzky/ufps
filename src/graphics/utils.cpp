#include "graphics/utils.h"

#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma GCC diagnostic pop

#include "graphics/texture_data.h"
#include "utils/data_buffer.h"
#include "utils/ensure.h"

namespace
{
    auto channels_to_format(int num_channels) -> ufps::TextureFormat
    {
        switch (num_channels)
        {
            using enum ufps::TextureFormat;
        case 1:
            return R;
        case 3:
            return RGB;
        case 4:
            return RGBA;
        default:
            throw ufps::Exception("unsupported channel count: {}", num_channels);
        }
    }
}

namespace ufps
{
    auto load_texture(DataBufferView image_data) -> TextureData
    {
        auto width = int{};
        auto height = int{};
        auto num_channels = int{};

        ::stbi_set_flip_vertically_on_load(true);

        auto raw_data = std::unique_ptr<::stbi_uc, void (*)(void *)>{
            ::stbi_load_from_memory(
                reinterpret_cast<const ::stbi_uc *>(image_data.data()),
                image_data.size(),
                &width,
                &height,
                &num_channels,
                0),
            ::stbi_image_free};

        ensure(raw_data, "failed to parse texture data");

        const auto *ptr = reinterpret_cast<const std::byte *>(raw_data.get());

        return {
            .width = static_cast<std::uint32_t>(width),
            .height = static_cast<std::uint32_t>(height),
            .format = channels_to_format(num_channels),
            .data = {ptr, ptr + width * height * num_channels},
        };
    }
}