#pragma once

#include <array>
#include <span>

#include "math/matrix4.h"
#include "math/vector3.h"
#include "utils/data_buffer.h"

namespace ufps
{

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
        Camera(const Vector3 &postion, const Vector3 &look_at, const Vector3 &up,
               float fov, float width, float height, float near_plane, float far_plane);

        Camera(float width, float height, float depth);

        auto direction() const -> Vector3;
        auto right() const -> Vector3;
        auto up() const -> Vector3;
        auto position() const -> Vector3;
        auto set_position(const Vector3 &position) -> void;

        auto translate(const Vector3 &translation) -> void;

        auto set_pitch(float value) -> void;
        auto set_yaw(float value) -> void;
        auto adjust_pitch(float value) -> void;
        auto adjust_yaw(float value) -> void;

        auto fov() const -> float;
        auto width() const -> float;
        auto height() const -> float;
        auto near_plane() const -> float;
        auto far_plane() const -> float;

        auto frustum_corners() const -> std::array<Vector3, 8u>;

        // auto invert_pitch(bool invert) -> void;
        // auto invert_yaw(bool invert) -> void;

        auto data() const -> const CameraData &;
        auto data_view() const -> DataBufferView;

        auto update() -> void;

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

        auto recalculate_view() -> void;
        auto clamp_pitch() -> void;
    };
}
