#pragma once

#include <string>

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
        MouseButtonEvent(float x, float y, MouseButtonState state);

        auto x() const -> float;
        auto y() const -> float;
        auto state() const -> MouseButtonState;

        auto to_string() const -> std::string;

    private:
        float _x;
        float _y;
        MouseButtonState _state;
    };

    auto to_string(MouseButtonState obj) -> std::string;

}
