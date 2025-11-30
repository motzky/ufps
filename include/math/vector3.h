#pragma once

#include <cmath>

#include "log.h"
#include "utils/formatter.h"

namespace ufps
{
    struct Vector3
    {
        constexpr Vector3()
            : Vector3(0.f)
        {
        }

        constexpr Vector3(float scalar)
            : Vector3(scalar, scalar, scalar)
        {
        }

        constexpr Vector3(float x, float y, float z)
            : x(x), y(y), z(z)
        {
        }

        float x;
        float y;
        float z;

        constexpr auto length() const -> float
        {
            return std::hypot(x, y, z);
        }

        static constexpr auto normalize(const Vector3 &v) -> Vector3
        {
            const auto length = v.length();

            if (length == 0.f)
            {
                return {};
            }

            return {v.x / length, v.y / length, v.z / length};
        }

        static constexpr auto cross(const Vector3 &v1, const Vector3 &v2) -> Vector3
        {
            const auto i = (v1.y * v2.z) - (v1.z * v2.y);
            const auto j = (v1.x * v2.z) - (v1.z * v2.x);
            const auto k = (v1.x * v2.y) - (v1.y * v2.x);

            return {i, -j, k};
        }

        static constexpr auto dot(const Vector3 &v1, const Vector3 &v2) -> float
        {
            return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
        }

        static auto distance(const Vector3 &v1, const Vector3 &v2) -> float;

        constexpr auto operator==(const Vector3 &) const -> bool = default;

        auto to_string() const -> std::string;
    };

    constexpr auto operator-=(Vector3 &v1, const Vector3 &v2) -> Vector3 &
    {
        v1.x -= v2.x;
        v1.y -= v2.y;
        v1.z -= v2.z;

        return v1;
    }
    constexpr auto operator-(const Vector3 &v1, const Vector3 &v2) -> Vector3
    {
        auto tmp = v1;
        return tmp -= v2;
    }
    constexpr auto operator+=(Vector3 &v1, const Vector3 &v2) -> Vector3 &
    {
        v1.x += v2.x;
        v1.y += v2.y;
        v1.z += v2.z;

        return v1;
    }
    constexpr auto operator+(const Vector3 &v1, const Vector3 &v2) -> Vector3
    {
        auto tmp = v1;
        return tmp += v2;
    }
    constexpr auto operator*=(Vector3 &v1, const float scale) -> Vector3 &
    {
        v1.x *= scale;
        v1.y *= scale;
        v1.z *= scale;

        return v1;
    }
    constexpr auto operator*(const Vector3 &v1, const float scale) -> Vector3
    {
        auto tmp = v1;
        return tmp *= scale;
    }
    constexpr auto operator/=(Vector3 &v1, const float scale) -> Vector3 &
    {
        v1.x /= scale;
        v1.y /= scale;
        v1.z /= scale;

        return v1;
    }
    constexpr auto operator/(const Vector3 &v1, const float scale) -> Vector3
    {
        auto tmp = v1;
        return tmp /= scale;
    }
    constexpr auto operator-(const Vector3 &v) -> Vector3
    {
        return {-v.x, -v.y, -v.z};
    }
}
