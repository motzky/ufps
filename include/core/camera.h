#pragma once

#include <array>
#include <span>

#include "math/matrix4.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "utils/data_buffer.h"

namespace ufps
{
    namespace impl
    {
        constexpr auto extract_up(const ufps::Transform &transform) -> ufps::Vector3
        {
            const auto m = ufps::Matrix4{transform};
            return ufps::Vector3::normalize({m[4zu], m[5zu], m[6zu]});
        }

        constexpr auto extract_right(const ufps::Transform &transform) -> ufps::Vector3
        {
            const auto m = ufps::Matrix4{transform};
            return -ufps::Vector3::normalize({m[0zu], m[1zu], m[2zu]});
        }

        constexpr auto extract_direction(const ufps::Transform &transform) -> ufps::Vector3
        {
            const auto m = ufps::Matrix4{transform};
            return ufps::Vector3::normalize({m[8zu], m[9zu], m[10zu]});
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
        struct Description
        {
            Transform transform;
            float fov;
            float width;
            float height;
            float near_plane;
            float far_plane;
        };

        constexpr Camera(const Description &description);

        constexpr Camera(const Transform &transform,
                         float fov, float width, float height, float near_plane, float far_plane);

        constexpr Camera(float width, float height, float depth);

        constexpr auto direction() const -> Vector3;
        constexpr auto up() const -> Vector3;
        constexpr auto right() const -> Vector3;

        constexpr auto transform() const -> const Transform &;
        constexpr auto set_transform(const Transform &transform) -> void;
        constexpr auto set_parent_transform(const Transform &transform) -> void;
        constexpr auto parent_transform() const -> const Transform &;

        constexpr auto fov() const -> float;
        constexpr auto width() const -> float;
        constexpr auto height() const -> float;
        constexpr auto near_plane() const -> float;
        constexpr auto far_plane() const -> float;

        constexpr auto frustum_corners() const -> std::array<Vector3, 8u>;

        constexpr auto data() const -> const CameraData &;
        constexpr auto data_view() const -> DataBufferView;

        constexpr auto description() const -> Description;

    private:
        constexpr auto update_transforms(const Transform &local, const Transform &parent) -> void;

        Vector3 _direction;
        Vector3 _up;
        Vector3 _right;

        Transform _local_transform;
        Transform _parent_transform;
        Transform _transform;

        float _fov;
        float _width;
        float _height;
        float _near_plane;
        float _far_plane;

        CameraData _data;

        constexpr auto recalculate_view() -> void;
        constexpr auto clamp_pitch() -> void;
    };

    constexpr Camera::Camera(const Description &description)
        : Camera{
              description.transform,
              description.fov,
              description.width,
              description.height,
              description.near_plane,
              description.far_plane}
    {
    }

    constexpr Camera::Camera(const Transform &transform,
                             float fov, float width, float height, float near_plane, float far_plane)
        : _direction(impl::extract_direction(transform)),
          _up(impl::extract_up(transform)),
          _right(impl::extract_right(transform)),
          _fov(fov),
          _width(width),
          _height(height),
          _near_plane(near_plane),
          _far_plane(far_plane),
          _data{.view = Matrix4::look_at(transform.position, _direction, _up),
                .projection = Matrix4::perspective(fov, width, height, near_plane, far_plane),
                .position = transform.position}
    {
    }

    constexpr Camera::Camera(float width, float height, float depth)
        : _direction{Vector3{0.f, 0.f, -1.f}},
          _up{Vector3{0.f, 1.f, 0.f}},
          _right{},
          _fov(0.f),
          _width(width),
          _height(height),
          _near_plane(0.001f),
          _far_plane(depth),
          _data{.view = Matrix4::look_at({0.f, 0.f, 1.f}, {}, {0.f, 1.f, 0.f}),
                .projection = Matrix4::orthographic(width, height, depth),
                .position = Vector3{0.f, 0.f, 1.f}}
    {
    }

    constexpr auto Camera::direction() const -> Vector3
    {
        return _direction;
    }
    constexpr auto Camera::up() const -> Vector3
    {
        return _up;
    }
    constexpr auto Camera::right() const -> Vector3
    {
        return _right;
    }
    constexpr auto Camera::transform() const -> const Transform &
    {
        return _transform;
    }
    constexpr auto Camera::set_transform(const Transform &transform) -> void
    {
        update_transforms(transform, _parent_transform);
    }

    constexpr auto Camera::parent_transform() const -> const Transform &
    {
        return _parent_transform;
    }

    constexpr auto Camera::data() const -> const CameraData &
    {
        return _data;
    }

    constexpr auto Camera::data_view() const -> DataBufferView
    {
        return {reinterpret_cast<const std::byte *>(&_data), sizeof(_data)};
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

    constexpr auto Camera::update_transforms(const Transform &local, const Transform &parent) -> void
    {
        _transform = parent * local;
        _local_transform = local;
        _parent_transform = parent;

        _direction = impl::extract_direction(_transform);
        _up = impl::extract_up(_transform);
        _right = impl::extract_right(_transform);
        _data.position = _transform.position;
        _data.view = Matrix4::look_at(_data.position, _data.position + _direction, _up);
    }

    constexpr auto Camera::set_parent_transform(const Transform &transform) -> void
    {
        update_transforms(_local_transform, transform);
    }

    constexpr auto Camera::description() const -> Description
    {
        return {
            .transform = _transform,
            .fov = _fov,
            .width = _width,
            .height = _height,
            .near_plane = _near_plane,
            .far_plane = _far_plane};
    }
}
