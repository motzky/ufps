#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include "audio/audio_backend.h"

namespace ufps
{
    // Cross-platform OpenAL implementation of AudioBackend.
    // Selected by the platform-specific factory in posix/win32 audio_backend.cpp,
    // so a future platform can swap this for WASAPI/ALSA without touching AudioManager.
    class OpenALAudioBackend final : public AudioBackend
    {
    public:
        OpenALAudioBackend();
        ~OpenALAudioBackend() override;

        auto create_buffer(DataBufferView data, int sample_rate, int channels, int bit_depth) -> std::uint32_t override;
        auto create_source(std::uint32_t buffer) -> std::uint32_t override;
        auto play_source(std::uint32_t source) -> void override;
        auto stop_source(std::uint32_t source) -> void override;
        auto delete_source(std::uint32_t source) -> void override;
        auto delete_buffer(std::uint32_t buffer) -> void override;

    private:
        ALCdevice *_device{};
        ALCcontext *_context{};
    };
}
