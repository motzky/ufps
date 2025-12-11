#include <iostream>
#include <numbers>
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
#include "graphics/mesh_data.h"
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

namespace
{
    auto cube() -> ufps::MeshData
    {
        const ufps::Vector3 positions[] = {
            {-1.f, -1.f, 1.f},
            {1.f, -1.f, 1.f},
            {1.f, 1.f, 1.f},
            {-1.f, 1.f, 1.f},
            {1.f, -1.f, 1.f},
            {1.f, -1.f, -1.f},
            {1.f, 1.f, -1.f},
            {1.f, 1.f, 1.f},
            {1.f, -1.f, -1.f},
            {-1.f, -1.f, -1.f},
            {-1.f, 1.f, -1.f},
            {1.f, 1.f, -1.f},
            {-1.f, -1.f, -1.f},
            {-1.f, -1.f, 1.f},
            {-1.f, 1.f, 1.f},
            {-1.f, 1.f, -1.f},
            {-1.f, 1.f, 1.f},
            {1.f, 1.f, 1.f},
            {1.f, 1.f, -1.f},
            {-1.f, 1.f, -1.f},
            {-1.f, -1.f, -1.f},
            {1.f, -1.f, -1.f},
            {1.f, -1.f, 1.f},
            {-1.f, -1.f, 1.f}};

        auto indices = std::vector<std::uint32_t>{
            // Each face: 2 triangles (6 indices)
            // Front face
            0, 1, 2, 2, 3, 0,
            // Right face
            4, 5, 6, 6, 7, 4,
            // Back face
            8, 9, 10, 10, 11, 8,
            // Left face
            12, 13, 14, 14, 15, 12,
            // Top face
            16, 17, 18, 18, 19, 16,
            // Bottom face
            20, 21, 22, 22, 23, 20};

        return {.vertices = positions |
                            std::views::transform([](const auto &e)
                                                  { return ufps::VertexData{.position = e, .color = ufps::Color::green()}; }) |
                            std::ranges::to<std::vector>(),
                .indices = std::move(indices)};
    }
}

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

            auto scene = ufps::Scene{
                .entities = {},
                .mesh_manager = mesh_manager,
                .camera = {
                    {},
                    {0.f, 0.f, -1.f},
                    {0.f, 1.f, 0.f},
                    std::numbers::pi_v<float> / 4.f,
                    static_cast<float>(window.width()),
                    static_cast<float>(window.height()),
                    0.01f,
                    1000.f}};

            scene.entities.push_back({.mesh_view = mesh_manager.load(cube())});

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
                                if (arg.key() == ufps::Key::ESC && arg.state() == ufps::KeyState::UP)
                                {
                                    running = false;
                                }
                            }
                        },
                        *event);

                    event = window.pump_event();
                }

                scene.camera.translate({0.f, 0.f, 0.01f});
                scene.camera.update();

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