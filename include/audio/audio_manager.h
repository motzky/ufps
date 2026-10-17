#pragma once

#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

// FAudio's public header uses a C99 flexible array member, which -pedantic rejects.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <FAudio.h>
#pragma GCC diagnostic pop

#include "audio/track.h"
#include "log.h"
#include "resources/resource_loader.h"
#include "utils/string_unordered_map.h"

namespace ufps
{
    enum class PlayMode
    {
        SINGLE,
        LOOP
    };

    class AudioManager
    {
    public:
        AudioManager(ResourceLoader &resource_loader);

        auto play(std::string_view track_name, PlayMode mode = PlayMode::SINGLE) -> void;

    private:
        static constexpr auto destroy_faudio = [](auto *obj)
        { ::FAudio_Release(obj); };

        static constexpr auto destroy_voice = [](auto *obj)
        { ::FAudioVoice_DestroyVoice(obj); };

        // A decoded track plus the dedicated source voice used to play it. The voice
        // is created once, at load time, from the track's own WAVE `fmt` chunk so each
        // sound keeps its true bit depth, channel count and sample rate. FAudio
        // resamples every source voice up to the mastering rate, so tracks with
        // different formats can all coexist.
        struct LoadedTrack
        {
            Track track;
            std::unique_ptr<::FAudioSourceVoice, decltype(destroy_voice)> voice;
        };

        StringUnorderedMap<LoadedTrack> _tracks;
        std::unique_ptr<::FAudio, decltype(destroy_faudio)> _xaudio;
        std::unique_ptr<::FAudioMasteringVoice, decltype(destroy_voice)> _mastering_voice;
    };

}
