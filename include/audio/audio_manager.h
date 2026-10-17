#pragma once

#include <memory>
#include <string_view>
#include <vector>

// FAudio's public header uses a C99 flexible array member, which -pedantic rejects.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <FAudio.h>
#pragma GCC diagnostic pop

#include "audio/track.h"
#include "resources/resource_loader.h"
#include "utils/string_unordered_map.h"

namespace ufps
{
    class AudioManager
    {
    public:
        AudioManager(ResourceLoader &resource_loader);

    private:
        static constexpr auto destroy_faudio = [](auto *obj)
        { FAudio_Release(obj); };

        static constexpr auto destroy_voice = [](auto *obj)
        { FAudioVoice_DestroyVoice(obj); };

        StringUnorderedMap<Track> _tracks;
        std::unique_ptr<::FAudio, decltype(destroy_faudio)> _xaudio;
        std::unique_ptr<::FAudioMasteringVoice, decltype(destroy_voice)> _mastering_voice;
        std::array<std::unique_ptr<::FAudioSourceVoice, decltype(destroy_voice)>, 64zu> _voices;
    };

}
