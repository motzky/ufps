#include "core/flycam_actor.h"

#include "core/actor.h"
#include "core/camera.h"
#include "events/key_map.h"
#include "math/vector3.h"

namespace
{
    auto walk_direction(const ufps::KeyMap &key_map, const ufps::Camera &camera) -> ufps::Vector3
    {
        auto direction = ufps::Vector3{};

        const auto camera_direction = camera.direction();
        const auto forward = ufps::Vector3::normalize({camera_direction.x, 0.f, camera_direction.z});

        if (key_map[ufps::Key::W])
        {
            direction += forward;
        }
        if (key_map[ufps::Key::S])
        {
            direction -= forward;
        }
        if (key_map[ufps::Key::D])
        {
            direction += camera.right();
        }
        if (key_map[ufps::Key::A])
        {
            direction -= camera.right();
        }
        if (key_map[ufps::Key::SPACE])
        {
            direction += camera.up();
        }
        if (key_map[ufps::Key::LCTRL])
        {
            direction -= camera.up();
        }

        auto factor = 96.f;
        if (key_map[ufps::Key::LSHIFT])
        {
            factor /= 4.f;
        }

        return direction / factor;
    }
}

namespace ufps
{
    FlyCamActor::FlyCamActor(Camera camera, const KeyMap &key_map)
        : Actor{std::move(camera)},
          _key_map{key_map}
    {
    }

    auto FlyCamActor::update() -> void
    {
        if (_key_map.delta_x != 0.f)
        {
            _camera.adjust_yaw(-_key_map.delta_x);
        }
        if (_key_map.delta_y != 0.f)
        {
            _camera.adjust_pitch(_key_map.delta_y);
        }

        _camera.translate(walk_direction(_key_map, _camera));
        _camera.update();
    }
}