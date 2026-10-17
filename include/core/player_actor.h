#pragma once

#include <tuple>
#include <vector>

#include "core/actor.h"
#include "core/camera_manager.h"
#include "core/clock.h"
#include "core/entity.h"
#include "core/entity_manager.h"
#include "core/scene.h"
#include "events/input_map.h"
#include "graphics/color.h"
#include "math/vector3.h"
#include "physics/virtual_character_controller.h"

namespace ufps
{
    class PlayerActor : public Actor
    {
    public:
        PlayerActor(
            EntityHandle entity,
            const InputMap &input_map,
            VirtualCharacterController &character_controller,
            const Scene &scene);
        ~PlayerActor() override = default;

        auto update(Duration delta) -> void override;

    private:
        const InputMap &_input_map;
        VirtualCharacterController &_character_controller;
        std::vector<std::tuple<Vector3, Vector3, Color>> _pew_pew_lines;
        const Scene &_scene;
        Duration _walk_sound_timer;
    };

}