#include "openal_audio_backend.h"

#include <stdexcept>
#include <string_view>

#include "log.h"

namespace ufps
{
    namespace
    {
        auto check_al(std::string_view operation) -> void
        {
            if (const auto error = alGetError(); error != AL_NO_ERROR)
            {
                log::warn("OpenAL: {} failed with error {}", operation, static_cast<int>(error));
            }
        }
    }

    OpenALAudioBackend::OpenALAudioBackend()
    {
        _device = alcOpenDevice(nullptr);
        if (!_device)
        {
            throw std::runtime_error("failed to open default OpenAL device");
        }

        _context = alcCreateContext(_device, nullptr);
        if (!_context)
        {
            alcCloseDevice(_device);
            _device = nullptr;
            throw std::runtime_error("failed to create OpenAL context");
        }

        if (alcMakeContextCurrent(_context) != AL_TRUE)
        {
            alcDestroyContext(_context);
            _context = nullptr;
            alcCloseDevice(_device);
            _device = nullptr;
            throw std::runtime_error("failed to make OpenAL context current");
        }

        log::info("OpenAL audio device initialized");
    }

    OpenALAudioBackend::~OpenALAudioBackend()
    {
        alcMakeContextCurrent(nullptr);
        if (_context)
        {
            alcDestroyContext(_context);
        }
        if (_device)
        {
            alcCloseDevice(_device);
        }
    }

    auto OpenALAudioBackend::create_buffer(DataBufferView data, int sample_rate, int channels, int bit_depth) -> std::uint32_t
    {
        ALuint buffer{};
        alGenBuffers(1, &buffer);
        check_al("alGenBuffers");

        const auto format = [&]
        {
            if (channels == 1)
            {
                return bit_depth == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
            }
            return bit_depth == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
        }();

        alBufferData(buffer, format, data.data(), static_cast<ALsizei>(data.size()), sample_rate);
        check_al("alBufferData");

        return static_cast<std::uint32_t>(buffer);
    }

    auto OpenALAudioBackend::create_source(std::uint32_t buffer) -> std::uint32_t
    {
        ALuint source{};
        alGenSources(1, &source);
        check_al("alGenSources");

        alSourcei(source, AL_BUFFER, static_cast<ALint>(buffer));
        check_al("alSourcei");

        return static_cast<std::uint32_t>(source);
    }

    auto OpenALAudioBackend::play_source(std::uint32_t source) -> void
    {
        alSourcePlay(static_cast<ALuint>(source));
        check_al("alSourcePlay");
    }

    auto OpenALAudioBackend::stop_source(std::uint32_t source) -> void
    {
        alSourceStop(static_cast<ALuint>(source));
        check_al("alSourceStop");
    }

    auto OpenALAudioBackend::delete_source(std::uint32_t source) -> void
    {
        const ALuint handle = static_cast<ALuint>(source);
        alDeleteSources(1, &handle);
        check_al("alDeleteSources");
    }

    auto OpenALAudioBackend::delete_buffer(std::uint32_t buffer) -> void
    {
        const ALuint handle = static_cast<ALuint>(buffer);
        alDeleteBuffers(1, &handle);
        check_al("alDeleteBuffers");
    }
}
