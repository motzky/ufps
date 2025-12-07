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
#include "graphics/buffer.h"
#include "graphics/command_buffer.h"
#include "graphics/entity.h"
#include "graphics/mesh_manager.h"
#include "graphics/multi_buffer.h"
#include "graphics/persistent_buffer.h"
#include "graphics/program.h"
#include "graphics/scene.h"
#include "graphics/shader.h"
#include "graphics/vertex_data.h"
#include "log.h"
#include "utils/data_buffer.h"
#include "utils/exception.h"
#include "window.h"

using namespace std::literals;

namespace
{

    constexpr auto sample_vertex_shader = R"(
#version 460 core

struct VertexData
{
    float position[3];
    float color[3];
};
    
layout(binding = 0, std430) readonly buffer vertices
{
    VertexData data[];
};

vec3 get_position(int index)
{
    return vec3(
        data[index].position[0],
        data[index].position[1],
        data[index].position[2]
    );
}

vec3 get_color(int index)
{
    return vec3(
        data[index].color[0],
        data[index].color[1],
        data[index].color[2]
    );
}

layout (location = 0) out vec3 out_color;

void main()
{
    gl_Position = vec4(get_position(gl_VertexID), 1.0);
    out_color = get_color(gl_VertexID);
}

    )"sv;

    constexpr auto sample_fragment_shader = R"(
#version 460 core


layout(location = 0) in vec3 in_color;
layout(location = 0) out vec4 color;

void main()
{
    color = vec4(in_color, 1.0);
}
    )"sv;

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

            const auto sample_vert = ufps::Shader{sample_vertex_shader, ufps::ShaderType::VERTEX, "sample_vertex_shader"sv};
            const auto sample_frag = ufps::Shader{sample_fragment_shader, ufps::ShaderType::FRAGMENT, "sample_fragement_shader"sv};

            auto sample_program = ufps::Program{sample_vert, sample_frag, "sample_program"sv};

            auto mesh_manager = ufps::MeshManager{};
            auto command_buffer = ufps::CommandBuffer{};

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

            auto dummy_vao = ufps::AutoRelease<::GLuint>{0u, [](auto e)
                                                         { ::glDeleteBuffers(1, &e); }};
            ::glGenVertexArrays(1u, &dummy_vao);

            ::glBindVertexArray(dummy_vao);
            ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh_manager.native_handle());

            sample_program.use();

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

                auto command_count = command_buffer.build(scene);

                ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, command_buffer.native_handle());

                ::glMultiDrawArraysIndirect(GL_TRIANGLES, nullptr, command_count, 0);

                command_buffer.advance();

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