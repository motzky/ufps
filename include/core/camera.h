#pragma once

#include <array>
#include <span>

#include "math/matrix4.h"
#include "math/vector3.h"
#include "utils/data_buffer.h"

namespace ufps
{
    namespace impl
    {
        constexpr auto create_direction(float pitch, float yaw) -> ufps::Vector3
        {
            return ufps::Vector3::normalize({std::cos(yaw) * std::cos(pitch),
                                             std::sin(pitch),
                                             std::sin(yaw) * std::cos(pitch)});
        }

    }
    struct CameraData
    {
        Matrix4 view;
        Matrix4 projection;
        Vector3 position;
        float _padding = 0.f;
    };

    class Camera
    {
    public:
        constexpr Camera(const Vector3 &postion, const Vector3 &look_at, const Vector3 &up,
                         float fov, float width, float height, float near_plane, float far_plane);

        constexpr Camera(float width, float height, float depth);

        constexpr auto direction() const -> Vector3;
        constexpr auto right() const -> Vector3;
        constexpr auto up() const -> Vector3;
        constexpr auto position() const -> Vector3;
        constexpr auto set_position(const Vector3 &position) -> void;

        constexpr auto translate(const Vector3 &translation) -> void;

        constexpr auto set_pitch(float value) -> void;
        constexpr auto set_yaw(float value) -> void;
        constexpr auto adjust_pitch(float value) -> void;
        constexpr auto adjust_yaw(float value) -> void;

        constexpr auto fov() const -> float;
        constexpr auto width() const -> float;
        constexpr auto height() const -> float;
        constexpr auto near_plane() const -> float;
        constexpr auto far_plane() const -> float;

        constexpr auto frustum_corners() const -> std::array<Vector3, 8u>;

        // auto invert_pitch(bool invert) -> void;
        // auto invert_yaw(bool invert) -> void;
        constexpr auto data() const -> const CameraData &;
        constexpr auto data_view() const -> DataBufferView;

        constexpr auto update() -> void;

    private:
        CameraData _data;
        Vector3 _direction;
        Vector3 _up;
        Vector3 _right;
        float _pitch;
        float _yaw;
        float _fov;
        float _width;
        float _height;
        float _near_plane;
        float _far_plane;

        // bool _invert_pitch;
        // bool _invert_yaw;

        constexpr auto recalculate_view() -> void;
        constexpr auto clamp_pitch() -> void;
    };

    constexpr Camera::Camera(const Vector3 &position, const Vector3 &look_at, const Vector3 &up,
                             float fov, float width, float height, float near_plane, float far_plane)
        : _data{.view = Matrix4::look_at(position, look_at, up),
                .projection = Matrix4::perspective(fov, width, height, near_plane, far_plane),
                .position = position},
          _direction(look_at),
          _up(up),
          _right(),
          _pitch{},
          _yaw{-std::numbers::pi_v<float> / 2.f},
          _fov(fov),
          _width(width),
          _height(height),
          _near_plane(near_plane),
          _far_plane(far_plane)
    {
        _direction = impl::create_direction(_pitch, _yaw);
        _right = Vector3::normalize(Vector3::cross(_direction, _up));
    }

    constexpr Camera::Camera(float width, float height, float depth)
        : _data{.view = Matrix4::look_at({0.f, 0.f, 1.f}, {}, {0.f, 1.f, 0.f}),
                .projection = Matrix4::orthographic(width, height, depth),
                .position = Vector3{0.f, 0.f, 1.f}},
          _direction{Vector3{0.f, 0.f, -1.f}},
          _up{Vector3{0.f, 1.f, 0.f}},
          _right{},
          _pitch{},
          _yaw{-std::numbers::pi_v<float> / 2.f},
          _fov(0.f),
          _width(width),
          _height(height),
          _near_plane(0.001f),
          _far_plane(depth)
    {
    }

    constexpr auto Camera::direction() const -> Vector3
    {
        return _direction;
    }
    constexpr auto Camera::right() const -> Vector3
    {
        return _right;
    }
    constexpr auto Camera::up() const -> Vector3
    {
        return _up;
    }
    constexpr auto Camera::position() const -> Vector3
    {
        return _data.position;
    }

    constexpr auto Camera::set_position(const Vector3 &position) -> void
    {
        _data.position = position;
    }

    constexpr auto Camera::translate(const Vector3 &translation) -> void
    {
        _data.position += translation;
        _direction += translation;
    }

    constexpr auto Camera::set_pitch(float value) -> void
    {
        _pitch = value;
    }
    constexpr auto Camera::set_yaw(float value) -> void
    {
        _yaw = value;
    }

    constexpr auto Camera::adjust_pitch(float value) -> void
    {
        // if (_invert_pitch)
        // {
        //     _pitch -= adjust;
        //     return;
        // }
        _pitch += value;

        clamp_pitch();
    }
    constexpr auto Camera::adjust_yaw(float value) -> void
    {
        // if (_invert_yaw)
        // {
        //     _yaw -= adjust;
        //     return;
        // }
        _yaw += value;
    }

    // constexpr auto Camera::invert_pitch(bool invert) -> void
    // {
    //     _invert_pitch = invert;
    // }
    // constexpr auto Camera::invert_yaw(bool invert) -> void
    // {
    //     _invert_yaw = invert;
    // }

    constexpr auto Camera::data() const -> const CameraData &
    {
        return _data;
    }

    constexpr auto Camera::data_view() const -> DataBufferView
    {
        return {reinterpret_cast<const std::byte *>(&_data), sizeof(_data)};
    }

    constexpr auto Camera::update() -> void
    {
        _direction = impl::create_direction(_pitch, _yaw);

        auto world_up = Vector3{0.f, 1.0f, 0.f};
        _right = Vector3::normalize(Vector3::cross(_direction, world_up));
        _up = Vector3::normalize(Vector3::cross(_right, _direction));
        recalculate_view();
    }

    constexpr auto Camera::fov() const -> float
    {
        return _fov;
    }

    constexpr auto Camera::width() const -> float
    {
        return _width;
    }

    constexpr auto Camera::height() const -> float
    {
        return _height;
    }

    constexpr auto Camera::near_plane() const -> float
    {
        return _near_plane;
    }

    constexpr auto Camera::far_plane() const -> float
    {
        return _far_plane;
    }

    constexpr auto Camera::recalculate_view() -> void
    {
        _data.view = Matrix4::look_at(_data.position, _data.position + _direction, _up);
    }

    constexpr auto Camera::frustum_corners() const -> std::array<Vector3, 8u>
    {
        auto corners = std::array<Vector3, 8u>{};

        const auto tan_half_fov = std::tan(_fov / 2.0f);
        const auto aspect = _width / _height;

        const auto near_height = 2.0f * tan_half_fov * _near_plane;
        const auto near_width = near_height * aspect;

        const auto far_height = 2.0f * tan_half_fov * _far_plane;
        const auto far_width = far_height * aspect;

        const auto forward = Vector3::normalize(_direction);
        const auto right = Vector3::normalize(Vector3::cross(forward, _up));
        const auto up = Vector3::normalize(Vector3::cross(right, forward));

        const auto near_center = _data.position + _direction * _near_plane;
        corners[0] = near_center + up * (near_height / 2.0f) - right * (near_width / 2.0f);
        corners[1] = near_center + up * (near_height / 2.0f) + right * (near_width / 2.0f);
        corners[2] = near_center - up * (near_height / 2.0f) + right * (near_width / 2.0f);
        corners[3] = near_center - up * (near_height / 2.0f) - right * (near_width / 2.0f);

        const auto far_center = _data.position + _direction * _far_plane;
        corners[4] = far_center + up * (far_height / 2.0f) - right * (far_width / 2.0f);
        corners[5] = far_center + up * (far_height / 2.0f) + right * (far_width / 2.0f);
        corners[6] = far_center - up * (far_height / 2.0f) + right * (far_width / 2.0f);
        corners[7] = far_center - up * (far_height / 2.0f) - right * (far_width / 2.0f);

        return corners;
    }

    constexpr auto Camera::clamp_pitch() -> void
    {
        constexpr auto pitch_eps = 0.0001f;
        _pitch = std::clamp(_pitch, (-std::numbers::pi_v<float> / 2.f) + pitch_eps, (std::numbers::pi_v<float> / 2.f) - pitch_eps);
    }

}
