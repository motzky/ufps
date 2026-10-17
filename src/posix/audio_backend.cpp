#include "audio/audio_backend.h"

#include "../audio/openal_audio_backend.h"
#include "log.h"

namespace ufps
{
    // POSIX audio backend factory. Currently uses the shared OpenAL implementation;
    // can be swapped for an ALSA/PulseAudio backend without touching AudioManager.
    auto create_audio_backend() -> std::unique_ptr<AudioBackend>
    {
        try
        {
            return std::make_unique<OpenALAudioBackend>();
        }
        catch (const std::exception &e)
        {
            log::error("failed to create audio backend: {}", e.what());
            return nullptr;
        }
    }
}
