#include "math/vector3.h"

#include <format>
#include <string>

namespace ufps
{
    auto Vector3::distance(const Vector3 &v1, const Vector3 &v2) -> float
    {
        return (v2 - v1).length();
    }

    auto Vector3::to_string() const -> std::string
    {
        return std::format("x={} y={} z={}", x, y, z);
    }

}