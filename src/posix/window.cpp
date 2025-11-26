#ifdef WIN32
#error This code unit is NOT for Windows
#endif

#include "window.h"

#include <cstdint>
#include <optional>
#include <print>
#include <queue>
#include <ranges>
#include <string_view>

#include <GLFW/glfw3.h>

#include "events/event.h"
#include "events/key.h"
#include "events/key_event.h"
#include "events/mouse_button_event.h"
#include "events/mouse_event.h"
#include "events/stop_event.h"
#include "graphics/opengl.h"
#include "log.h"
#include "utils/auto_release.h"
#include "utils/ensure.h"

namespace
{
    auto g_event_queue = std::queue<ufps::Event>{};
    auto g_process_mouse_events = false;

    auto gl_source_to_string(GLenum source) -> std::string_view
    {
        switch (source)
        {
        case GL_DEBUG_SOURCE_API:
            return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            return "Window System";
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            return "Shader Compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            return "Third party";
        case GL_DEBUG_SOURCE_APPLICATION:
            return "APP";
        case GL_DEBUG_SOURCE_OTHER:
            return "OTHER";

        default:
            return std::to_string(source);
        }
    }

    auto gl_message_type_to_string(GLenum type) -> std::string_view
    {
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR:
            return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            return "DEPRECATED BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            return "UNDEFINED  BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY:
            return "PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE:
            return "PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER:
            return "MARKER";
        case GL_DEBUG_TYPE_PUSH_GROUP:
            return "PUSH GROUP";
        case GL_DEBUG_TYPE_POP_GROUP:
            return "POP GROUP";
        case GL_DEBUG_TYPE_OTHER:
            return "OTHER";

        default:
            return std::to_string(type);
        }
    }

    void APIENTRY opengl_debug_callback(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei,
        const GLchar *message,
        const void *)
    {
        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH:
            ufps::log::error("{} - {} - {}: {}", gl_source_to_string(source), id, gl_message_type_to_string(type), message);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            ufps::log::warn("{} - {} - {}: {}", gl_source_to_string(source), id, gl_message_type_to_string(type), message);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            ufps::log::debug("{} - {} - {}: {}", gl_source_to_string(source), id, gl_message_type_to_string(type), message);
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            if (source == GL_DEBUG_SOURCE_SHADER_COMPILER && type == GL_DEBUG_TYPE_OTHER)
            {
                // just ignore shader compiler stats
                return;
            }
            ufps::log::debug("{} - {} - {}: {}", gl_source_to_string(source), id, gl_message_type_to_string(type), message);
            break;
        }
    }

    void APIENTRY glfw_error_callback(int, const char *description)
    {
        ufps::log::error("{}", description);
    }

    auto key_callback([[maybe_unused]] GLFWwindow *window, int key, int, int action, int) -> void
    {
        if (action == GLFW_PRESS)
        {
            g_event_queue.emplace(ufps::KeyEvent{static_cast<ufps::Key>(key), ufps::KeyState::DOWN});
            return;
        }
        if (action == GLFW_RELEASE)
        {
            g_event_queue.emplace(ufps::KeyEvent{static_cast<ufps::Key>(key), ufps::KeyState::UP});
            return;
        }
    }

    auto mouse_button_callback(GLFWwindow *window, int button, int action, int /*mods*/) -> void
    {
        if (!g_process_mouse_events)
        {
            return;
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            auto state = action == GLFW_PRESS ? ufps::MouseButtonState::DOWN : ufps::MouseButtonState::UP;

            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            g_event_queue.emplace(ufps::MouseButtonEvent{static_cast<float>(xpos), static_cast<float>(ypos), state});
        }
    }

    auto mouse_cursor_entered_left_callback(GLFWwindow *, int entered) -> void
    {
        if (entered)
        {
            g_process_mouse_events = true;
        }
        else
        {
            g_process_mouse_events = false;
        }
    }

    auto mouse_callback(GLFWwindow * /*window*/, double xpos, double ypos) -> void
    {
        if (!g_process_mouse_events)
        {
            return;
        }
        static auto last_x = float{static_cast<float>(xpos)};
        static auto last_y = float{static_cast<float>(ypos)};

        auto xf = static_cast<float>(xpos);
        auto yf = static_cast<float>(ypos);

        auto dx = last_x - xf;
        auto dy = last_y - yf;

        g_event_queue.emplace(ufps::MouseEvent{dx, dy});

        last_x = xf;
        last_y = yf;
    }

    template <class T>
    auto resolve_gl_function(T &function, const std::string &name) -> void
    {
        const auto address = ::GL_GET_PROC_ADDRESS(name.c_str());
        ufps::ensure(address != nullptr, "failed to resolve {}", name);

        function = reinterpret_cast<T>(address);
    }

    auto resolve_global_gl_functions() -> void
    {
#define RESOLVE(TYPE, NAME) resolve_gl_function(NAME, #NAME);
        FOR_OPENGL_FUNCTIONS(RESOLVE);
    }

    auto setup_debug() -> void
    {
        ::glEnable(GL_DEBUG_OUTPUT);
        // ::glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        ::glDebugMessageCallback(opengl_debug_callback, nullptr);

        ::glfwSetErrorCallback(glfw_error_callback);
    }
}

namespace ufps
{
    Window::Window(std::uint32_t width, std::uint32_t height, std::uint32_t x, std::uint32_t y, std::uint8_t samples)
        : _windowHandle{}, _width(width), _height(height)
    {
        log::info("Running on *nix, creating window with GLFW");

        ufps::ensure(glfwInit(), "failed to initialize GLFW");

        ::glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        ::glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

        if (x != 0 && y != 0)
        {
            ::glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        }

        if (samples > 1)
        {
            ::glfwWindowHint(GLFW_SAMPLES, samples);
        }

        _windowHandle = {::glfwCreateWindow(width, height, "Game Window", nullptr, nullptr),
                         ::glfwDestroyWindow};
        ufps::ensure(_windowHandle, "failed to create window");

        if (x != 0 && y != 0)
        {
            ::glfwSetWindowPos(_windowHandle, x, y);
            ::glfwShowWindow(_windowHandle);
        }

        log::debug("registering input callbacks...");

        ::glfwSetKeyCallback(_windowHandle, key_callback);
        if (::glfwRawMouseMotionSupported())
        {
            // ::glfwSetInputMode(_windowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            ::glfwSetInputMode(_windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            ::glfwSetInputMode(_windowHandle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
        else
        {
            ::glfwSetInputMode(_windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        ::glfwSetCursorEnterCallback(_windowHandle, mouse_cursor_entered_left_callback);
        ::glfwSetCursorPosCallback(_windowHandle, mouse_callback);
        ::glfwSetMouseButtonCallback(_windowHandle, mouse_button_callback);
        // TODO
        // ::glfwSetScrollCallback(_windowHandle, scroll_callback);

        ::glfwMakeContextCurrent(_windowHandle);

        log::debug("resolving OpenGL functions...");

        ::resolve_global_gl_functions();
        ::setup_debug();

        ::glEnable(GL_DEPTH_TEST);

        if (samples > 1)
        {
            ::glEnable(GL_MULTISAMPLE);
        }

        auto vendor_str = ::glGetString(GL_VENDOR);
        ufps::log::info("Current render device vendor: {}", reinterpret_cast<const char *>(vendor_str));

        auto renderer_str = ::glGetString(GL_RENDERER);
        ufps::log::info("Current render device: {}", reinterpret_cast<const char *>(renderer_str));
    }

    auto Window::pump_event() const -> std::optional<Event>
    {
        ::glfwPollEvents();

        if (::glfwWindowShouldClose(_windowHandle))
        {
            return g_event_queue.emplace(StopEvent{});
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
        ::glfwSwapBuffers(_windowHandle);
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
        ::glfwSetWindowTitle(_windowHandle, title.c_str());
    }

    auto Window::show_cursor(bool show) const -> void
    {
        ::glfwSetInputMode(_windowHandle, GLFW_CURSOR, show ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }
}
