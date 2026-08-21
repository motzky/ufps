#include "core/player_actor.h"

#include "core/actor.h"
#include "core/camera.h"
#include "core/service_locator.h"
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
    PlayerActor::PlayerActor(CameraHandle camera, EntityHandle player_entity, const InputMap &key_map, VirtualCharacterController &character_controller)
        : Actor{camera},
          _input_map{key_map},
          _character_controller{character_controller},
          _player_entity{player_entity}
    {
    }

    auto PlayerActor::update() -> void
    {
        const auto &[em, cm] = services<EntityManager, CameraManager>();

        const auto camera = cm[_camera];
        contract_assert(camera);

        static auto yaw = 0.f;
        static auto pitch = 0.f;

        yaw += _input_map.delta_x;
        pitch -= _input_map.delta_y;

        constexpr auto pitch_eps = 0.0001f;
        pitch = std::clamp(pitch, (-std::numbers::pi_v<float> / 2.f) + pitch_eps, (std::numbers::pi_v<float> / 2.f) - pitch_eps);

        _character_controller.set_walk_direction(walk_direction(_input_map, *camera));

        auto player = em[_player_entity];
        contract_assert(player);

        const auto new_transform = Transform{_character_controller.position(), {1.f}, Quaternion{yaw, pitch, 0.f}};
        player->set_transform(new_transform);
    }
}