#pragma once

#include <cstdint>
#include <memory>

#include "utils/data_buffer.h"

namespace ufps
{
    class AudioBackend
    {
    public:
        virtual ~AudioBackend() = default;

        virtual auto create_buffer(DataBufferView data, int sample_rate, int channels, int bit_depth) -> std::uint32_t = 0;
        virtual auto create_source(std::uint32_t buffer) -> std::uint32_t = 0;
        virtual auto play_source(std::uint32_t source) -> void = 0;
        virtual auto stop_source(std::uint32_t source) -> void = 0;
        virtual auto delete_source(std::uint32_t source) -> void = 0;
        virtual auto delete_buffer(std::uint32_t buffer) -> void = 0;
    };

    // Implemented per platform (src/posix/audio_backend.cpp, src/win32/audio_backend.cpp).
    // Returns nullptr when no audio device is available.
    auto create_audio_backend() -> std::unique_ptr<AudioBackend>;
}
