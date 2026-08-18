#pragma once

#include "core/actor.h"
#include "core/camera.h"
#include "core/entity.h"
#include "core/entity_manager.h"
#include "events/input_map.h"
#include "physics/virtual_character_controller.h"

namespace ufps
{
    class PlayerActor : public Actor
    {
    public:
        PlayerActor(Camera camera, EntityHandle player_entity, const InputMap &key_map, VirtualCharacterController &character_controller);
        ~PlayerActor() override = default;

        auto update() -> void override;

    private:
        const InputMap &_key_map;
        VirtualCharacterController &_character_controller;
        EntityHandle _player_entity;
    };

}