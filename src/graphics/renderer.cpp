#include "graphics/renderer.h"

#include <ranges>
#include <span>
#include <string_view>

#include "core/camera.h"
#include "core/scene.h"
#include "graphics/command_buffer.h"
#include "graphics/object_data.h"
#include "graphics/opengl.h"
#include "graphics/program.h"
#include "graphics/shader.h"
#include "graphics/utils.h"
#include "utils/auto_release.h"

using namespace std::literals;

namespace
{

    constexpr auto sample_vertex_shader = R"(
#version 460 core

struct VertexData
{
    float position[3];
};

struct ObjectData
{
    mat4 model;
    uint material_index;
};

struct MaterialData
{
    float color[3];
};

layout(binding = 0, std430) readonly buffer vertices
{
    VertexData data[];
};

layout(binding = 1, std430) readonly buffer camera
{
    mat4 view;
    mat4 projection;
};

layout(binding = 2, std430) readonly buffer objects
{
    ObjectData object_data[];
};

layout(binding = 3, std430) readonly buffer materials
{
    MaterialData material_data[];
};

vec3 get_position(uint index)
{
    return vec3(
        data[index].position[0],
        data[index].position[1],
        data[index].position[2]
    );
}

layout (location = 0) out flat uint material_index;

void main()
{
    gl_Position = projection * view * object_data[gl_DrawID].model * vec4(get_position(gl_VertexID), 1.0);
    material_index = object_data[gl_DrawID].material_index;
}

)"sv;

    constexpr auto sample_fragment_shader = R"(
#version 460 core

struct MaterialData
{
    float color[3];
};

layout(binding = 3, std430) readonly buffer materials
{
    MaterialData material_data[];
};

layout(location = 0) in flat uint material_index;
layout(location = 0) out vec4 color;

vec3 get_color(uint index)
{
    return vec3(
        material_data[index].color[0],
        material_data[index].color[1],
        material_data[index].color[2]
    );
}

void main()
{
    color = vec4(get_color(material_index), 1.0);
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
          _camera_buffer{sizeof(CameraData), "camera_buffer"},
          _object_data_buffer{sizeof(ObjectData), "object_data_buffer"},
          _program{create_program()}
    {
        ::glGenVertexArrays(1u, &_dummy_vao);

        ::glBindVertexArray(_dummy_vao);

        _program.use();
    }

    auto Renderer::render(const Scene &scene) -> void
    {
        _camera_buffer.write(scene.camera.data_view(), 0zu);

        const auto [vertex_buffer_handle, index_buffer_handle] = scene.mesh_manager.native_handle();
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertex_buffer_handle);
        ::glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, _camera_buffer.native_handle(), _camera_buffer.frame_offset_bytes(), sizeof(CameraData));
        ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_handle);

        const auto command_count = _command_buffer.build(scene);

        ::glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _command_buffer.native_handle());

        const auto object_data = scene.entities |
                                 std::views::transform([&scene](const auto &e)
                                                       { 
                                                        const auto index = scene.material_manager.index(e.material_key);
                                                         return ObjectData{
                                                            .model = e.transform, 
                                                            .material_id_index = index,
                                                             .padding={},}; }) |
                                 std::ranges::to<std::vector>();

        resize_gpu_buffer(object_data, _object_data_buffer, "object_data_buffer");

        _object_data_buffer.write(std::as_bytes(std::span{object_data.data(), object_data.size()}), 0zu);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _object_data_buffer.native_handle());

        scene.material_manager.sync();
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, scene.material_manager.native_handle());

        ::glMultiDrawElementsIndirect(GL_TRIANGLES,
                                      GL_UNSIGNED_INT,
                                      reinterpret_cast<const void *>(_command_buffer.offset_bytes()),
                                      command_count,
                                      0);

        _command_buffer.advance();
        _camera_buffer.advance();
        _object_data_buffer.advance();
        scene.material_manager.advance();
    }
}
