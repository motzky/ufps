#include "core/player_actor.h"

#include "core/actor.h"
#include "core/camera.h"
#include "events/input_map.h"
#include "math/vector3.h"

namespace
{
    auto walk_direction(const ufps::InputMap &input_map, const ufps::Camera &camera) -> ufps::Vector3
    {
        auto direction = ufps::Vector3{};

        const auto camera_direction = camera.direction();
        const auto forward = ufps::Vector3::normalize({camera_direction.x, 0.f, camera_direction.z});

        if (input_map[ufps::Key::W])
        {
            direction += forward;
        }
        if (input_map[ufps::Key::S])
        {
            direction -= forward;
        }
        if (input_map[ufps::Key::D])
        {
            direction += camera.right();
        }
        if (input_map[ufps::Key::A])
        {
            direction -= camera.right();
        }
        // if (key_map[ufps::Key::SPACE])
        // {
        //     direction += camera.up();
        // }
        // if (key_map[ufps::Key::LCTRL])
        // {
        //     direction -= camera.up();
        // }

        // auto factor = 96.f;
        // if (key_map[ufps::Key::LSHIFT])
        // {
        //     factor /= 4.f;
        // }

        // return direction / factor;
        return ufps::Vector3::normalize(direction);
    }

}

namespace ufps
{
    PlayerActor::PlayerActor(Camera camera, const InputMap &key_map, VirtualCharacterController &character_controller)
        : Actor{std::move(camera)},
          _key_map{key_map},
          _character_controller{character_controller}
    {
    }

    auto PlayerActor::update() -> void
    {
        if (_key_map.delta_x != 0.f)
        {
            _camera.adjust_yaw(-_key_map.delta_x);
        }
        if (_key_map.delta_y != 0.f)
        {
            _camera.adjust_pitch(_key_map.delta_y);
        }

        _character_controller.set_walk_direction(walk_direction(_key_map, _camera));
        _camera.set_position(_character_controller.position() + Vector3{0.f, 2.f, 0.f});
        _camera.update();
    }
}