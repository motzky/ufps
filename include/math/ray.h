#pragma once

#include "math/vector3.h"

namespace ufps
{

    class Ray
    {
    public:
        constexpr Ray(const Vector3 &origin, const Vector3 &direction)
            : origin{origin}, direction{Vector3::normalize(direction)}
        {
        }

        Vector3 origin;
        Vector3 direction;
    };

}