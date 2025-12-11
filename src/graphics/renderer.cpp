#include "graphics/renderer.h"

#include <string_view>

#include "graphics/command_buffer.h"
#include "graphics/opengl.h"
#include "graphics/program.h"
#include "graphics/scene.h"
#include "graphics/shader.h"
#include "utils/auto_release.h"

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

    auto create_program() -> ufps::Program
    {
        const auto sample_vert = ufps::Shader{sample_vertex_shader, ufps::ShaderType::VERTEX, "sample_vertex_shader"sv};
        const auto sample_frag = ufps::Shader{sample_fragment_shader, ufps::ShaderType::FRAGMENT, "sample_fragement_shader"sv};

        return ufps::Program{sample_vert, sample_frag, "sample_program"sv};
    }
}

namespace ufps
{
    Renderer::Renderer()
        : _dummy_vao{0u, [](auto e)
                     { ::glDeleteBuffers(1, &e); }},
          _command_buffer{},
          _program{create_program()}
    {
        ::glGenVertexArrays(1u, &_dummy_vao);

        ::glBindVertexArray(_dummy_vao);

        _program.use();
    }

    auto Renderer::render(const Scene &scene) -> void
    {
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, scene.mesh_manager.native_handle());

        const auto command_count = _command_buffer.build(scene);

        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _command_buffer.native_handle());

        ::glMultiDrawArraysIndirect(GL_TRIANGLES,
                                    reinterpret_cast<const void *>(_command_buffer.offset_bytes()),
                                    command_count,
                                    0);

        _command_buffer.advance();
    }
}
