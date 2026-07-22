#pragma once

#include "core/actor.h"
#include "core/camera.h"
#include "events/key_map.h"
#include "physics/virtual_character_controller.h"

namespace ufps
{
    class PlayerActor : public Actor
    {
    public:
        PlayerActor(Camera camera, const KeyMap &key_map, VirtualCharacterController &character_controller);
        ~PlayerActor() override = default;

        auto update() -> void override;

    private:
        const KeyMap &_key_map;
        VirtualCharacterController &_character_controller;
    };

}