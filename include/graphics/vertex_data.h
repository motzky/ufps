#pragma once

#include "graphics/color.h"
#include "math/vector3.h"

namespace ufps
{
    struct VertexData
    {
        Vector3 position;
        Color color;
    };

    static_assert(sizeof(VertexData) == sizeof(float) * 3 + sizeof(float) * 3);

}