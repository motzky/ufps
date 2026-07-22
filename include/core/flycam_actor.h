#pragma once

#include "core/actor.h"
#include "core/camera.h"
#include "events/input_map.h"

namespace ufps
{
    class FlyCamActor : public Actor
    {
    public:
        FlyCamActor(Camera camera, const InputMap &key_map);
        ~FlyCamActor() override = default;

        auto update() -> void override;

    private:
        const InputMap &_key_map;
    };

}