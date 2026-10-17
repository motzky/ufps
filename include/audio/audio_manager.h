#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "audio/audio_backend.h"
#include "audio/track.h"
#include "resources/resource_loader.h"
#include "utils/string_unordered_map.h"

namespace ufps
{
    class AudioManager
    {
    public:
        AudioManager(ResourceLoader &resource_loader);
        ~AudioManager();

        auto play(std::string_view track_name) -> void;
        auto stop(std::string_view track_name) -> void;

    private:
        StringUnorderedMap<Track> _tracks;
        std::unique_ptr<AudioBackend> _backend;
    };

}
