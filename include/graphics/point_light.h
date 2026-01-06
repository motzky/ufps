#pragma once

#include "graphics/color.h"
#include "math/vector3.h"

namespace ufps
{
    struct PointLight
    {
        Vector3 position;
        Color color;
        float constant_attenuation;
        float linear_attenuation;
        float quadratic_attenuation;
    };

    static_assert(sizeof(PointLight) == sizeof(float) * 9);

}