#include "core/player_actor.h"

#include "audio/audio_manager.h"
#include "core/actor.h"
#include "core/camera.h"
#include "core/scene.h"
#include "core/service_locator.h"
#include "events/input_map.h"
#include "graphics/color.h"
#include "graphics/debug_layer.h"
#include "math/ray.h"
#include "math/vector3.h"

using namespace std::literals;

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
    PlayerActor::PlayerActor(
        EntityHandle entity,
        const InputMap &input_map,
        VirtualCharacterController &character_controller,
        const Scene &scene)
        : Actor{entity},
          _input_map{input_map},
          _character_controller{character_controller},
          _scene{scene},
          _walk_sound_timer{}
    {
        service<CameraHandle>() = _camera;
    }

    auto PlayerActor::update(Duration delta) -> void
    {
        const auto &[em, cm, dl, am] = services<EntityManager, CameraManager, DebugLayer, AudioManager>();

        _shoot_timer += delta;
        _walk_sound_timer += delta;

        if (_input_map.is_any_set<Key::W, Key::S, Key::A, Key::D>())
        {
            if (_walk_sound_timer >= 600ms)
            {
                am.play("walk.wav");
                _walk_sound_timer = {};
            }
        }

        const auto camera = cm[_camera];
        contract_assert(camera);

        static auto yaw = std::numbers::pi_v<float>;
        static auto pitch = 0.f;

        yaw += _input_map.delta_x;
        pitch -= _input_map.delta_y;

        constexpr auto pitch_eps = 0.0001f;
        pitch = std::clamp(pitch, (-std::numbers::pi_v<float> / 2.f) + pitch_eps, (std::numbers::pi_v<float> / 2.f) - pitch_eps);

        _character_controller.set_walk_direction(walk_direction(_input_map, *camera));

        auto player = em[_entity];
        contract_assert(player);

        const auto transform = player->transform();

        auto new_transform = Transform{_character_controller.position(), {1.f}, Quaternion{yaw, pitch, 0.f}};
        new_transform.position.y = transform.position.y;

        player->set_transform(new_transform);

        if (const auto mouse_event = _input_map.mouse_event; mouse_event && mouse_event->state() == MouseButtonState::DOWN)
        {
            if (_shoot_timer >= 600ms)
            {
                _shoot_timer = {};

                am.play("single-laser-shot.wav");

                const auto bullet_ray = Ray{camera->transform().position, camera->direction() * 100.f};

                if (const auto intersection = _scene.intersect_ray(bullet_ray); intersection)
                {
                    _pew_pew_lines.push_back(std::make_tuple(intersection->position, intersection->position + (intersection->normal * 0.5f), Color::blue()));

                    _pew_pew_lines.push_back(std::make_tuple(bullet_ray.origin, bullet_ray.origin + (bullet_ray.direction * intersection->distance), Color::hot_pink()));
                }
                else
                {
                    _pew_pew_lines.push_back(std::make_tuple(bullet_ray.origin, bullet_ray.origin + (bullet_ray.direction * 100.f), Color::red()));
                }
            }
        }

        for (const auto &[start, end, color] : _pew_pew_lines)
        {
            dl.push_line(start, end, color, DebugLayerType::DEFAULT);
        }
    }
}