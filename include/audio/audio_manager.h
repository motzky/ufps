#pragma once

#include <vector>

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
        StringUnorderedMap<Track> _tracks;
    };

}
