#include "core/flycam_actor.h"

#include "core/actor.h"
#include "core/camera.h"
#include "events/input_map.h"
#include "math/vector3.h"

namespace
{
    [[maybe_unused]] auto walk_direction(const ufps::InputMap &input_map, const ufps::Camera &camera) -> ufps::Vector3
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
        if (input_map[ufps::Key::SPACE])
        {
            direction += camera.up();
        }
        if (input_map[ufps::Key::LCTRL])
        {
            direction -= camera.up();
        }

        auto factor = 96.f;
        if (input_map[ufps::Key::LSHIFT])
        {
            factor /= 4.f;
        }

        return direction / factor;
    }
}

namespace ufps
{
    FlyCamActor::FlyCamActor(CameraHandle camera, EntityHandle entity, const InputMap &key_map)
        : Actor{camera},
          _input_map{key_map},
          _entity{entity}
    {
    }

    auto FlyCamActor::update() -> void
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

        auto e = em[_entity];
        contract_assert(e);

        static const auto speed = 1.0f;
        const auto transform = e->transform();
        const auto new_transform = Transform{transform.position + (walk_direction(_input_map, *camera) * speed), {1.f}, Quaternion{pitch, yaw, 0.f}};

        e->set_transform(new_transform);
    }
}