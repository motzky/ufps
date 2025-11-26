#ifndef _WIN32
#error This code unit is ONLY for Windows !
#endif

#include "window.h"

#include <cstdint>
#include <optional>
#include <print>
#include <queue>
#include <ranges>

#include "graphics/opengl.h"

#include "opengl/wglext.h"
#include <Windows.h>
#include <Windowsx.h>
#include <hidusage.h>

#include "events/event.h"
#include "events/key.h"
#include "events/key_event.h"
#include "events/mouse_button_event.h"
#include "events/mouse_event.h"
#include "events/stop_event.h"
#include "log.h"
#include "utils/auto_release.h"
#include "utils/ensure.h"

#pragma comment(lib, "opengl32.lib")

namespace
{

    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB{};
    PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB{};

    auto g_event_queue = std::queue<ufps::Event>{};

    void APIENTRY opengl_debug_callback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei,
        const GLchar *message,
        const void *)
    {
        if (type == GL_DEBUG_TYPE_ERROR)
        {
            ufps::ensure(false, "{} {} {} {} {}", source, type, id, severity, message);
        }
        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH:
            ufps::log::error("{} - {}: {} from {}", id, type, message, source);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            ufps::log::warn("{} - {}: {} from {}", id, type, message, source);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            ufps::log::info("{} - {}: {} from {}", id, type, message, source);
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            ufps::log::debug("{} - {}: {} from {}", id, type, message, source);
            break;
        }
    }

    auto CALLBACK window_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) -> LRESULT
    {
        switch (Msg)
        {
        case WM_CLOSE:
            g_event_queue.emplace(ufps::StopEvent{});
            break;
        case WM_KEYUP:
        {
            g_event_queue.emplace(ufps::KeyEvent{static_cast<ufps::Key>(wParam), ufps::KeyState::UP});
            break;
        }
        case WM_KEYDOWN:
        {
            g_event_queue.emplace(ufps::KeyEvent{static_cast<ufps::Key>(wParam), ufps::KeyState::DOWN});
            break;
        }
        case WM_INPUT:
        {
            auto raw = ::RAWINPUT{};
            auto dwSize = UINT{sizeof(RAWINPUT)};
            ufps::ensure(::GetRawInputData(reinterpret_cast<::HRAWINPUT>(lParam), RID_INPUT, &raw, &dwSize, sizeof(::RAWINPUTHEADER)) != static_cast<::UINT>(-1), "failed to get raw input");

            if (raw.header.dwType == RIM_TYPEMOUSE)
            {
                const auto x = raw.data.mouse.lLastX;
                const auto y = raw.data.mouse.lLastY;

                g_event_queue.emplace(ufps::MouseEvent{static_cast<float>(x), static_cast<float>(y)});
            }
            break;
        }
        case WM_LBUTTONUP:
        {
            g_event_queue.emplace(ufps::MouseButtonEvent{
                static_cast<float>(GET_X_LPARAM(lParam)),
                static_cast<float>(GET_Y_LPARAM(lParam)),
                ufps::MouseButtonState::UP});
            break;
        }
        case WM_LBUTTONDOWN:
        {
            g_event_queue.emplace(ufps::MouseButtonEvent{
                static_cast<float>(GET_X_LPARAM(lParam)),
                static_cast<float>(GET_Y_LPARAM(lParam)),
                ufps::MouseButtonState::DOWN});
            break;
        }
        }

        return ::DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    template <class T>
    auto resolve_gl_function(T &function, const std::string &name) -> void
    {
        const auto address = ::GL_GET_PROC_ADDRESS(name.c_str());
        ufps::ensure(address != nullptr, "failed to resolve {}", name);

        function = reinterpret_cast<T>(address);
    }

    auto resolve_wgl_functions(HINSTANCE instance) -> void
    {
        auto wc = ::WNDCLASSA{
            .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
            .lpfnWndProc = ::DefWindowProc,
            .hInstance = instance,
            .lpszClassName = "dummy window"};

        ufps::ensure(::RegisterClassA(&wc) != 0, "faild to register dummy window class");

        auto dummy_window = ufps::AutoRelease<::HWND>{
            ::CreateWindowExA(
                0,
                wc.lpszClassName,
                wc.lpszClassName,
                0,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                wc.hInstance,
                0),
            ::DestroyWindow};

        ufps::ensure(dummy_window, "failed to create dummy window");

        auto dc = ufps::AutoRelease<HDC>{::GetDC(dummy_window), [&dummy_window](auto dc)
                                         { ::ReleaseDC(dummy_window, dc); }};
        ufps::ensure(dc, "failed to get dummy dc");

        auto pfd = ::PIXELFORMATDESCRIPTOR{
            .nSize = sizeof(::PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            .iPixelType = PFD_TYPE_RGBA,
            .cColorBits = 32,
            .cAlphaBits = 8,
            .cDepthBits = 24,
            .cStencilBits = 8,
            .iLayerType = PFD_MAIN_PLANE};

        auto pixel_format = ::ChoosePixelFormat(dc, &pfd);
        ufps::ensure(pixel_format != 0, "failed to choose pixel format");

        ufps::ensure(::SetPixelFormat(dc, pixel_format, &pfd) == TRUE, "failed to set pixel format");

        const auto context = ufps::AutoRelease<HGLRC>{::wglCreateContext(dc), ::wglDeleteContext};
        ufps::ensure(context, "failed to create wgl context");

        ufps::ensure(::wglMakeCurrent(dc, context) == TRUE, "failed to make current context");

        resolve_gl_function(wglCreateContextAttribsARB, "wglCreateContextAttribsARB");
        resolve_gl_function(wglChoosePixelFormatARB, "wglChoosePixelFormatARB");

        ufps::ensure(::wglMakeCurrent(dc, 0) == TRUE, "failed to unbind context");
    }

    auto init_opengl(HDC dc) -> void
    {
        int pixel_format_attribs[]{
            WGL_DRAW_TO_WINDOW_ARB,
            GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,
            GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,
            GL_TRUE,
            WGL_ACCELERATION_ARB,
            WGL_FULL_ACCELERATION_ARB,
            WGL_PIXEL_TYPE_ARB,
            WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,
            32,
            WGL_DEPTH_BITS_ARB,
            24,
            WGL_STENCIL_BITS_ARB,
            8,
            WGL_SAMPLE_BUFFERS_ARB,
            GL_FALSE,
            WGL_SAMPLES_ARB,
            0,
            0};

        auto pixel_format = 0;
        auto num_formats = UINT{};

        ::wglChoosePixelFormatARB(dc, pixel_format_attribs, 0, 1, &pixel_format, &num_formats);
        ufps::ensure(num_formats != 0u, "failed to choose a pixel format");

        auto pfd = ::PIXELFORMATDESCRIPTOR{};
        ufps::ensure(::DescribePixelFormat(dc, pixel_format, sizeof(pfd), &pfd) != 0, "failed to describe pixel format");
        ufps::ensure(::SetPixelFormat(dc, pixel_format, &pfd) == TRUE, "failed to set pixel format");

        int gl_attribts[]{
            WGL_CONTEXT_MAJOR_VERSION_ARB,
            4,
            WGL_CONTEXT_MINOR_VERSION_ARB,
            6,
            WGL_CONTEXT_PROFILE_MASK_ARB,
            WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0};

        auto context = ::wglCreateContextAttribsARB(dc, 0, gl_attribts);
        ufps::ensure(context != nullptr, "failed to create wgl context");

        ufps::ensure(::wglMakeCurrent(dc, context) == TRUE, "failed to make current context");
    }

    auto resolve_global_gl_functions() -> void
    {
#define RESOLVE(TYPE, NAME) resolve_gl_function(NAME, #NAME);
        FOR_OPENGL_FUNCTIONS(RESOLVE);
    }

    auto setup_debug() -> void
    {
        ::glEnable(GL_DEBUG_OUTPUT);
        ::glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        ::glDebugMessageCallback(opengl_debug_callback, nullptr);
    }
}

namespace ufps
{
    Window::Window(std::uint32_t width, std::uint32_t height, std::uint32_t x, std::uint32_t y)
        : _windowHandle({}), _width(width), _height(height),
          //
          _wc({}), _dc({})
    {
        log::info("running on Windows...");

        ensure(::CoInitializeEx(nullptr, COINIT_MULTITHREADED) == S_OK, "failed to initialize COM");

        _wc = {
            .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
            .lpfnWndProc = ::window_proc,
            .hInstance = ::GetModuleHandle(nullptr),
            .lpszClassName = "GameWindowClass",
        };

        ensure(::RegisterClassA(&_wc) != 0, "failed to register window class");

        ::RECT rect{
            .left = {},
            .top = {},
            .right = static_cast<int>(width),
            .bottom = static_cast<int>(height)};

        ensure(::AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false) != 0, "Failed to adjust window rectangle");

        _windowHandle = {::CreateWindowExA(
                             0,
                             _wc.lpszClassName,                                    // Class name, should be registered before this
                             "Game Window",                                        // Windwindow title
                             WS_OVERLAPPEDWINDOW,                                  // Window style
                             x, y, rect.right - rect.left, rect.bottom - rect.top, // Position and size
                             nullptr,                                              // Parent window
                             nullptr,                                              // Menu
                             _wc.hInstance,                                        // Instance handle
                             nullptr                                               // Additional application data
                             ),
                         ::DestroyWindow}; // Use AutoRelease to manage the window handle

        _dc = AutoRelease<HDC>{::GetDC(_windowHandle), [this](auto dc)
                               { ::ReleaseDC(_windowHandle, dc); }};

        ::ShowWindow(_windowHandle, SW_SHOW); // Show the window
        ::UpdateWindow(_windowHandle);        // Update the window to reflect changes

        const auto rid = ::RAWINPUTDEVICE{
            .usUsagePage = HID_USAGE_PAGE_GENERIC,
            .usUsage = HID_USAGE_GENERIC_MOUSE,
            .dwFlags = RIDEV_INPUTSINK,
            .hwndTarget = _windowHandle};

        ensure(::RegisterRawInputDevices(&rid, 1, sizeof(rid)) == TRUE, "failed to register input device");

        resolve_wgl_functions(_wc.hInstance);
        init_opengl(_dc);
        resolve_global_gl_functions();
        setup_debug();

        ::glEnable(GL_DEPTH_TEST);
        ::glEnable(GL_MULTISAMPLE);
    }

    auto Window::pump_event() const -> std::optional<Event>
    {
        auto message = ::MSG{};
        while (::PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE) != 0)
        {
            ::TranslateMessage(&message);
            ::DispatchMessage(&message);
        }

        if (!std::ranges::empty(g_event_queue))
        {
            const auto event = g_event_queue.front();
            g_event_queue.pop();
            return event;
        }

        return {};
    }

    auto Window::swap() const -> void
    {
        ::SwapBuffers(_dc);
    }

    auto Window::native_handle() const -> HandleType
    {
        return _windowHandle;
    }
    auto Window::width() const -> std::uint32_t
    {
        return _width;
    }
    auto Window::height() const -> std::uint32_t
    {
        return _height;
    }

    auto Window::set_title(const std::string &title) const -> void
    {
        ::SetWindowTextA(_windowHandle, title.c_str());
    }

    auto Window::show_cursor(bool /*show*/) const -> void
    {
        log::warn("show_cursor() not supported on Windows yet!");
    }

}
