#pragma once

#include "core/actor.h"
#include "core/camera_manager.h"
#include "core/entity_manager.h"
#include "events/input_map.h"

namespace ufps
{
    class FlyCamActor : public Actor
    {
    public:
        FlyCamActor(CameraHandle camera, EntityHandle entity, const InputMap &key_map);
        ~FlyCamActor() override = default;

        auto update() -> void override;

    private:
        const InputMap &_input_map;
        EntityHandle _entity;
    };

}