#pragma once

#include <format>
#include <string>

namespace ufps
{

    class MouseEvent
    {

    public:
        constexpr MouseEvent(float delta_x, float delta_y);

        constexpr auto delta_x() const -> float;
        constexpr auto delta_y() const -> float;

        constexpr auto to_string() const -> std::string;

    private:
        float _delta_x;
        float _delta_y;
    };

    constexpr MouseEvent::MouseEvent(float delta_x, float delta_y)
        : _delta_x(delta_x), _delta_y(delta_y)
    {
    }

    constexpr auto MouseEvent::delta_x() const -> float
    {
        return _delta_x;
    }

    constexpr auto MouseEvent::delta_y() const -> float
    {
        return _delta_y;
    }

    constexpr auto MouseEvent::to_string() const -> std::string
    {
        return std::format("MouseEvent {} {}", _delta_x, _delta_y);
    }
}
