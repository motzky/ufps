#pragma once

#include <cstdint>

#include "utils/data_buffer.h"

namespace ufps
{
    struct Track
    {
        DataBuffer format;
        DataBuffer data;

        std::uint32_t buffer_handle{};
        std::uint32_t source_handle{};
        int sample_rate{};
        int channels{};
        int bit_depth{};
    };
}