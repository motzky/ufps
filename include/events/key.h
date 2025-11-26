#pragma once

#include <format>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "log.h"

namespace ufps
{
    enum class Key
    {
        TAB = 0x9,
        RETURN = 0xd,

#ifdef _WIN32
        LSHIFT = 0x10,
        LCTRL = 0x11,
#else
        LSHIFT = 0x154,
        LCTRL = 0x155,
#endif

#ifdef _WIN32
        ESC = 0x1b,
#endif

        SPACE = 0x20,
        PG_UP = 0x21,
        PG_DOWN = 0x22,
        END = 0x23,
        HOME = 0x24,
#ifdef _WIN32
        LEFT_ARROW = 0x25,
        UP_ARROW = 0x26,
        RIGHT_ARROW = 0x27,
        DOWN_ARROW = 0x28,
#endif

        INSERT = 0x2d,
#ifdef _WIN32
        DELETE_KEY = 0x2e,
#else

        DELETE = 0x2e,
#endif

        KEY_0 = 0x30,
        KEY_1 = 0x31,
        KEY_2 = 0x32,
        KEY_3 = 0x33,
        KEY_4 = 0x34,
        KEY_5 = 0x35,
        KEY_6 = 0x36,
        KEY_7 = 0x37,
        KEY_8 = 0x38,
        KEY_9 = 0x39,

        A = 0x41,
        B = 0x42,
        C = 0x43,
        D = 0x44,
        E = 0x45,
        F = 0x46,
        G = 0x47,
        H = 0x48,
        I = 0x49,
        J = 0x4a,
        K = 0x4b,
        L = 0x4c,
        M = 0x4d,
        N = 0x4e,
        O = 0x4f,
        P = 0x50,
        Q = 0x51,
        R = 0x52,
        S = 0x53,
        T = 0x54,
        U = 0x55,
        V = 0x56,
        W = 0x57,
        X = 0x58,
        Y = 0x59,
        Z = 0x5a,

        NUMPAD_0 = 0x60,
        NUMPAD_1 = 0x61,
        NUMPAD_2 = 0x62,
        NUMPAD_3 = 0x63,
        NUMPAD_4 = 0x64,
        NUMPAD_5 = 0x65,
        NUMPAD_6 = 0x66,
        NUMPAD_7 = 0x67,
        NUMPAD_8 = 0x68,
        NUMPAD_9 = 0x69,

#ifndef WIN32
        ESC = 0x100,
#endif

#ifndef WIN32
        RIGHT_ARROW = 0x106,
        LEFT_ARROW = 0x107,
        DOWN_ARROW = 0x108,
        UP_ARROW = 0x109,
#endif

#ifdef _WIN32
        F1 = VK_F1,
        F2 = VK_F2,
        F3 = VK_F3,
        F4 = VK_F4,
        F5 = VK_F5,
        F6 = VK_F6,
        F7 = VK_F7,
        F8 = VK_F8,
        F9 = VK_F9,
        F10 = VK_F10,
        F11 = VK_F11,
        F12 = VK_F12,
#else
        F1 = 0x122,
        F2 = 0x123,
        F3 = 0x124,
        F4 = 0x125,
        F5 = 0x126,
        F6 = 0x127,
        F7 = 0x128,
        F8 = 0x129,
        F9 = 0x12a,
        F10 = 0x12b,
        F11 = 0x12c,
        F12 = 0x12d,
#endif

    };

    enum class KeyState
    {
        UP,
        DOWN
    };

    inline auto to_string(KeyState obj) -> std::string
    {
        switch (obj)
        {
            using enum ufps::KeyState;
        case UP:
            return "UP";
        case DOWN:
            return "DOWN";
        default:
            return "?";
        }
    }

    inline auto to_string(Key obj) -> std::string
    {

        switch (obj)
        {
            using enum ufps::Key;
        case TAB:
            return "TAB";
        case RETURN:
            return "RETURN";
        case LSHIFT:
            return "LSHIFT";
        case LCTRL:
            return "LCTRL";
        case ESC:
            return "ESC";
        case SPACE:
            return "SPACE";
        case PG_UP:
            return "PG_UP";
        case PG_DOWN:
            return "PG_DOWN";
        case END:
            return "END";
        case HOME:
            return "HOME";
        case LEFT_ARROW:
            return "LEFT_ARROW";
        case UP_ARROW:
            return "UP_ARROW";
        case RIGHT_ARROW:
            return "RIGHT_ARROW";
        case DOWN_ARROW:
            return "DOWN_ARROW";
        case INSERT:
            return "INSERT";
#ifdef _WIN32
        case DELETE_KEY:
#else
        case DELETE:
#endif
            return "DELETE";

        case KEY_0:
            return "0";
        case KEY_1:
            return "1";
        case KEY_2:
            return "2";
        case KEY_3:
            return "3";
        case KEY_4:
            return "4";
        case KEY_5:
            return "5";
        case KEY_6:
            return "6";
        case KEY_7:
            return "7";
        case KEY_8:
            return "8";
        case KEY_9:
            return "9";

        case NUMPAD_0:
            return "0";
        case NUMPAD_1:
            return "1";
        case NUMPAD_2:
            return "2";
        case NUMPAD_3:
            return "3";
        case NUMPAD_4:
            return "4";
        case NUMPAD_5:
            return "5";
        case NUMPAD_6:
            return "6";
        case NUMPAD_7:
            return "7";
        case NUMPAD_8:
            return "8";
        case NUMPAD_9:
            return "9";

        case A:
            return "A";
        case B:
            return "B";
        case C:
            return "C";
        case D:
            return "D";
        case E:
            return "E";
        case F:
            return "F";
        case G:
            return "G";
        case H:
            return "H";
        case I:
            return "I";
        case J:
            return "J";
        case K:
            return "K";
        case L:
            return "L";
        case M:
            return "M";
        case N:
            return "N";
        case O:
            return "O";
        case P:
            return "P";
        case Q:
            return "Q";
        case R:
            return "R";
        case S:
            return "S";
        case T:
            return "T";
        case U:
            return "U";
        case V:
            return "V";
        case W:
            return "W";
        case X:
            return "X";
        case Y:
            return "Y";
        case Z:
            return "Z";

        case F1:
            return "F1";
        case F2:
            return "F2";
        case F3:
            return "F3";
        case F4:
            return "F4";
        case F5:
            return "F5";
        case F6:
            return "F6";
        case F7:
            return "F7";
        case F8:
            return "F8";
        case F9:
            return "F9";
        case F10:
            return "F10";
        case F11:
            return "F11";
        case F12:
            return "F12";
        default:
            ufps::log::debug("key pressed raw: {}", static_cast<int>(obj));
            return "?";
        }
    }
}
