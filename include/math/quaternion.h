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

        constexpr Quaternion(const Vector3 &axis, float angle)
        {
            const auto sin_half = std::sin(angle / 2.f);
            const auto cos_half = std::cos(angle / 2.f);

            x = axis.x * sin_half;
            y = axis.y * sin_half;
            z = axis.z * sin_half;
            w = cos_half;

            *this = normalize(*this);
        }

        constexpr Quaternion(float yaw, float pitch, float roll);

        static constexpr auto normalize(const Quaternion &q) -> Quaternion;

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

    constexpr Quaternion::Quaternion(float yaw, float pitch, float roll)
    {
        const auto qx = Quaternion{{1.f, 0.f, 0.f}, pitch};
        const auto qy = Quaternion{{0.f, 1.f, 0.f}, yaw};
        const auto qz = Quaternion{{0.f, 0.f, 1.f}, roll};

        *this = normalize(qz * qy * qx);
    }

    constexpr auto Quaternion::normalize(const Quaternion &q) -> Quaternion
    {
        const auto det = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        contract_assert(det != 0.f);

        return {
            q.x / det,
            q.y / det,
            q.z / det,
            q.w / det};
    }

    inline auto Quaternion::to_string() const -> std::string
    {
        return std::format("x={} y={} z={} w={}", x, y, z, w);
    }
}
