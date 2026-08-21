#pragma once

#include "core/actor.h"
#include "core/camera_manager.h"
#include "core/entity.h"
#include "core/entity_manager.h"
#include "events/input_map.h"
#include "physics/virtual_character_controller.h"

namespace ufps
{
    class PlayerActor : public Actor
    {
    public:
        PlayerActor(EntityHandle entity, const InputMap &input_map, VirtualCharacterController &character_controller);
        ~PlayerActor() override = default;

        auto update() -> void override;

    private:
        const InputMap &_input_map;
        VirtualCharacterController &_character_controller;
    };

}