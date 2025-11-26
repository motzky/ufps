#pragma once

#include <cstdint>
#include <optional>

#ifdef _WIN32
#include <windows.h>
#else
#include <GLFW/glfw3.h>
#endif

#include "events/event.h"
#include "utils/auto_release.h"

namespace ufps
{
    class Window
    {
    public:
#ifdef _WIN32
        typedef ::HWND HandleType;
#else
        typedef ::GLFWwindow *HandleType;
#endif

        Window(std::uint32_t width, std::uint32_t height, std::uint32_t x, std::uint32_t y, std::uint8_t samples = 1);
        ~Window() = default;

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        Window(Window &&) noexcept = default;
        Window &operator=(Window &&) = default;

        auto pump_event() const -> std::optional<Event>;
        auto swap() const -> void;

        auto native_handle() const -> HandleType;
        auto width() const -> std::uint32_t;
        auto height() const -> std::uint32_t;

        auto set_title(const std::string &title) const -> void;

        auto show_cursor(bool show) const -> void;

    private:
        AutoRelease<HandleType, nullptr> _windowHandle;
        std::uint32_t _width;
        std::uint32_t _height;
#ifdef _WIN32
        AutoRelease<::HDC> _dc;
        ::WNDCLASSA _wc;
#endif
    };
}