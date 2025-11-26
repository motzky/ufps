#include "events/mouse_button_event.h"

#include <format>
#include <string>

#include "utils/formatter.h"

namespace ufps
{

    MouseButtonEvent::MouseButtonEvent(float x, float y, MouseButtonState state)
        : _x(x), _y(y), _state(state)
    {
    }

    auto MouseButtonEvent::x() const -> float
    {
        return _x;
    }
    auto MouseButtonEvent::y() const -> float
    {
        return _y;
    }
    auto MouseButtonEvent::state() const -> MouseButtonState
    {
        return _state;
    }

    auto MouseButtonEvent::to_string() const -> std::string
    {
        return std::format("MouseButtonEvent: {} @ {} {}", _state, _x, _y);
    }

    auto to_string(MouseButtonState obj) -> std::string
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
