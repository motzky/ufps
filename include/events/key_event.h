#pragma once

#include <format>

#include "events/key.h"
#include <utils/formatter.h>

namespace ufps
{
    class KeyEvent
    {

    public:
        constexpr KeyEvent(Key key, KeyState state);
        constexpr auto key() const -> Key;
        constexpr auto state() const -> KeyState;

        constexpr auto operator==(const KeyEvent &) const -> bool = default;
        constexpr auto to_string() const -> std::string;

    private:
        Key _key;
        KeyState _state;
    };

    constexpr KeyEvent::KeyEvent(Key key, KeyState state)
        : _key(key),
          _state(state)
    {
    }

    constexpr auto KeyEvent::key() const -> Key
    {
        return _key;
    }

    constexpr auto KeyEvent::state() const -> KeyState
    {
        return _state;
    }

    constexpr auto KeyEvent::to_string() const -> std::string
    {
        return std::format("KeyEvent {} {}", _key, _state);
    }
}
