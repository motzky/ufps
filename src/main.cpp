#include <iostream>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#ifndef WIN32
#include <GLFW/glfw3.h>
#endif

#include "config.h"
#include "graphics/command_buffer.h"
#include "graphics/mesh_manager.h"
#include "graphics/multi_buffer.h"
#include "graphics/persistent_buffer.h"
#include "graphics/renderer.h"
#include "graphics/scene.h"
#include "graphics/vertex_data.h"
#include "log.h"
#include "utils/data_buffer.h"
#include "utils/exception.h"
#include "window.h"

using namespace std::literals;

auto main(int argc, char **argv) -> int
{
    ufps::log::info("starting game {}.{}.{}", ufps::version::major, ufps::version::minor, ufps::version::patch);

    const auto args = std::vector<std::string_view>(argv + 1u, argv + argc);

#if defined(_MSC_VER) || defined(__GNUC__) && (__GNUC__ >= 15)
    // only msvc and GCC >=15 support automatic formatting of std::vector
    ufps::log::info("args: {}", args);
#else
    std::print("args: [");
    for (const auto &s : args)
    {
        std::print("\"{}\", ", s);
    }
    std::println("]");
#endif

    {

        try
        {
            auto window = ufps::Window{1920u, 1080u, 0u, 0u};
            auto running = true;

            auto mesh_manager = ufps::MeshManager{};
            auto renderer = ufps::Renderer{};

            auto scene = ufps::Scene{.entities = {}, .mesh_manager = mesh_manager};

            scene.entities.push_back(
                {.mesh_view = mesh_manager.load(
                     {{{0.f, 0.f, 0.f}, ufps::Color::red()},
                      {{-.5f, 0.f, 0.f}, ufps::Color::green()},
                      {{-.5f, .5f, 0.f}, ufps::Color::blue()}})});

            scene.entities.push_back(
                {.mesh_view = mesh_manager.load(
                     {{{0.f, 0.0f, 0.f}, ufps::Color::red()},
                      {{-.5f, .5f, 0.f}, ufps::Color::blue()},
                      {{0.f, .5f, 0.f}, ufps::Color::green()}})});

            while (running)
            {
                ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                auto event = window.pump_event();
                while (event && running)
                {
                    std::visit(
                        [&](auto &&arg)
                        {
                            using T = std::decay_t<decltype(arg)>;

                            if constexpr (std::same_as<T, ufps::StopEvent>)
                            {
                                running = false;
                            }
                            if constexpr (std::same_as<T, ufps::KeyEvent>)
                            {
                                running = false;
                            }
                        },
                        *event);

                    event = window.pump_event();
                }

                renderer.render(scene);

                window.swap();
            }
        }
        catch (const ufps::Exception &err)
        {
            std::println(std::cerr, "{}", err);
        }
        catch (...)
        {
            std::println(std::cerr, "unknown exception");
        }
    }

#ifndef WIN32
    ::glfwTerminate();
#endif

    return 0;
}