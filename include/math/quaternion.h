#pragma once

#include <cmath>
#include <format>

#include "math/vector3.h"

namespace ufps
{
    class Quaternion
    {
    public:
        constexpr Quaternion()
            : Quaternion(0.f, 0.f, 0.f, 1.f)
        {
        }

        constexpr Quaternion(float x, float y, float z, float w)
            : x{x},
              y{y},
              z{z},
              w{w}
        {
        }

        constexpr Quaternion(const Vector3 &v, float w)
            : x{v.x},
              y{v.y},
              z{v.z},
              w{w}
        {
        }

        auto to_string() const -> std::string;

        float x;
        float y;
        float z;
        float w;
    };

    constexpr auto operator*=(Quaternion &q1, const Quaternion &q2) -> Quaternion &
    {
        auto result = Quaternion{};
        result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
        result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
        result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
        result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;

        q1 = result;
        return q1;
    }

    constexpr auto operator*(const Quaternion &q1, const Quaternion &q2) -> Quaternion
    {
        auto tmp{q1};
        return tmp *= q2;
    }

    inline auto Quaternion::to_string() const -> std::string
    {
        return std::format("x={} y={} z={} w={}", x, y, z, w);
    }
}
