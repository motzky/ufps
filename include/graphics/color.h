#pragma once

#include <format>
#include <string>

namespace ufps
{
    struct Color
    {
        float r;
        float g;
        float b;

        static constexpr auto black() -> Color;
        static constexpr auto blue() -> Color;
        static constexpr auto green() -> Color;
        static constexpr auto red() -> Color;
        static constexpr auto white() -> Color;
        static constexpr auto yellow() -> Color;
        static constexpr auto magenta() -> Color;
    };

    inline constexpr auto Color::black() -> Color { return {.r = 0.f, .g = 0.f, .b = 0.f}; }
    inline constexpr auto Color::blue() -> Color { return {.r = 0.f, .g = 0.f, .b = 1.f}; }
    inline constexpr auto Color::green() -> Color { return {.r = 0.f, .g = 1.f, .b = 0.f}; }
    inline constexpr auto Color::red() -> Color { return {.r = 1.f, .g = 0.f, .b = 0.f}; }
    inline constexpr auto Color::white() -> Color { return {.r = 1.f, .g = 1.f, .b = 1.f}; }
    inline constexpr auto Color::yellow() -> Color { return {.r = 1.f, .g = 1.f, .b = 0.f}; }
    inline constexpr auto Color::magenta() -> Color { return {.r = 1.f, .g = 0.f, .b = 1.f}; }

    inline auto to_string(Color obj) -> std::string
    {
        return std::format("r={} g={} b={}", obj.r, obj.g, obj.b);
    }

}
