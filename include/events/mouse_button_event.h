#pragma once

#include <format>
#include <string>

#include "utils/formatter.h"

namespace ufps
{
    enum class MouseButtonState
    {
        UP,
        DOWN
    };

    class MouseButtonEvent
    {

    public:
        constexpr MouseButtonEvent(float x, float y, MouseButtonState state);

        constexpr auto x() const -> float;
        constexpr auto y() const -> float;
        constexpr auto state() const -> MouseButtonState;

        constexpr auto to_string() const -> std::string;

    private:
        float _x;
        float _y;
        MouseButtonState _state;
    };

    constexpr auto to_string(MouseButtonState obj) -> std::string;

    constexpr MouseButtonEvent::MouseButtonEvent(float x, float y, MouseButtonState state)
        : _x(x), _y(y), _state(state)
    {
    }

    constexpr auto MouseButtonEvent::x() const -> float
    {
        return _x;
    }
    constexpr auto MouseButtonEvent::y() const -> float
    {
        return _y;
    }
    constexpr auto MouseButtonEvent::state() const -> MouseButtonState
    {
        return _state;
    }

    constexpr auto MouseButtonEvent::to_string() const -> std::string
    {
        return std::format("MouseButtonEvent: {} @ {} {}", _state, _x, _y);
    }

    constexpr auto to_string(MouseButtonState obj) -> std::string
    {
        switch (obj)
        {
            using enum ufps::MouseButtonState;
        case UP:
            return "UP";
        case DOWN:
            return "DOWN";
        default:
            return "UNKNOWN";
        }
    }
}
