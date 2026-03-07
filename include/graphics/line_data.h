#pragma once

#include <format>

#include "graphics/color.h"
#include "math/vector3.h"

#include "utils/formatter.h"

namespace ufps
{
    struct LineData
    {
        Vector3 position;
        Color color;
    };

    inline auto to_string(const LineData &obj) -> std::string
    {
        return std::format("{} {}", obj.position, obj.color);
    }
}