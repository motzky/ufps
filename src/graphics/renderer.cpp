#include "graphics/renderer.h"

#include <ranges>
#include <span>
#include <string_view>

#include "core/camera.h"
#include "core/scene.h"
#include "graphics/command_buffer.h"
#include "graphics/object_data.h"
#include "graphics/opengl.h"
#include "graphics/point_light.h"
#include "graphics/program.h"
#include "graphics/shader.h"
#include "graphics/utils.h"
#include "utils/auto_release.h"

using namespace std::literals;

namespace
{

    constexpr auto sample_vertex_shader = R"glsl(
#version 460 core

struct VertexData
{
    float position[3];
    float normal[3];
    float tangent[3];
    float bitangent[3];
    float uv[2];
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

layout(binding = 4, std430) readonly buffer lights
{
    float ambient_color[3];
    float point_light_pos[3];
    float point_light_color[3];
    float point_light_attenuation[3];
};

vec3 get_position(uint index)
{
    return vec3(
        data[index].position[0],
        data[index].position[1],
        data[index].position[2]
    );
}
vec3 get_normal(uint index)
{
    return vec3(
        data[index].normal[0],
        data[index].normal[1],
        data[index].normal[2]
    );
}
vec3 get_tangent(uint index)
{
    return vec3(
        data[index].tangent[0],
        data[index].tangent[1],
        data[index].tangent[2]
    );
}
vec3 get_bitangent(uint index)
{
    return vec3(
        data[index].bitangent[0],
        data[index].bitangent[1],
        data[index].bitangent[2]
    );
}
vec2 get_uv(uint index)
{
    return vec2(data[index].uv[0],
                data[index].uv[1]);
}

layout (location = 0) out flat uint material_index;
layout (location = 1) out vec2 uv;
layout (location = 2) out vec4 frag_position;
layout (location = 3) out mat3 tbn;

void main()
{
    mat3 normal_mat = transpose(inverse(mat3(object_data[gl_DrawID].model)));

    frag_position = object_data[gl_DrawID].model * vec4(get_position(gl_VertexID), 1.0);
    gl_Position = projection * view * frag_position;
    material_index = object_data[gl_DrawID].material_index;
    uv = get_uv(gl_VertexID);

    vec3 t = normalize(vec3(object_data[gl_DrawID].model * vec4(get_tangent(gl_VertexID), 0.0)));
    vec3 b = normalize(vec3(object_data[gl_DrawID].model * vec4(get_bitangent(gl_VertexID), 0.0)));
    vec3 n = normalize(vec3(object_data[gl_DrawID].model * vec4(get_normal(gl_VertexID), 0.0)));

    tbn = mat3(t,b,n);
}

)glsl"sv;

    constexpr auto sample_fragment_shader = R"glsl(
#version 460 core
#extension GL_ARB_bindless_texture : require

struct VertexData
{
    float position[3];
    float normal[3];
    float tangent[3];
    float bitangent[3];
    float uv[2];
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

layout(binding = 0, std430) readonly buffer vertices {
    VertexData data[];
};

layout(binding = 1, std430) readonly buffer camera {
    mat4 view;
    mat4 projection;
};

layout(binding = 2, std430) readonly buffer objects {
    ObjectData object_data[];
};

layout(binding = 3, std430) readonly buffer materials
{
    MaterialData material_data[];
};

layout(binding = 4, std430) readonly buffer lights
{
    float ambient_color[3];
    float point_light_pos[3];
    float point_light_color[3];
    float point_light_attenuation[3];
};

layout(location = 0, bindless_sampler) uniform sampler2D albedo_tex;
layout(location = 1, bindless_sampler) uniform sampler2D normal_tex;

layout(location = 0) in flat uint material_index;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 frag_position;
layout(location = 3) in mat3 tbn;

layout(location = 0) out vec4 color;

vec3 get_color(uint index)
{
    return vec3(
        material_data[index].color[0],
        material_data[index].color[1],
        material_data[index].color[2]
    );
}

vec3 calc_point(vec3 frag_pos, vec3 n)
{
    vec3 pos = vec3(point_light_pos[0], point_light_pos[1], point_light_pos[2]);
    vec3 color = vec3(point_light_color[0], point_light_color[1], point_light_color[2]);
    vec3 attenuation = vec3(point_light_attenuation[0], point_light_attenuation[1], point_light_attenuation[2]);

    float dist = length(pos - frag_pos);
    float att = 1.0 / (attenuation.x + (attenuation.y * dist) + (attenuation.z * (dist * dist)));

    vec3 light_dir = normalize(pos - frag_pos);
    float diff = max(dot(n, light_dir), 0.0);

    return (diff * color);

    // vec3 reflect_dir = reflect(-light_dir, nn);
    // float spec = pow(max(dot(normalize(eye - frag_pos), reflect_dir), 0.0), 32) * texture(tex1, tex_coord).r;

    // return ((diff + spec) * att) * color;
}

void main()
{
    // need to flip V for textures...
    // should better be done on CPU during texture loading
    vec2 uv_inv = vec2(uv.x, -uv.y);
    vec3 nm = texture(normal_tex, uv_inv).xyz;
    nm = (nm*2.0 - 1.0);
    vec3 n = normalize(tbn * nm);

    vec3 albedo = texture(albedo_tex, uv_inv).rgb;
    vec3 ambient_col = vec3(ambient_color[0], ambient_color[1], ambient_color[2]);
    vec3 point_col = calc_point(frag_position.xyz, n);

    color = vec4(albedo * (ambient_col + point_col), 1.0);
}
)glsl"sv;

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
          _light_buffer{sizeof(LightData), "light_buffer"},
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

        _light_buffer.write(std::as_bytes(std::span<const LightData>{&scene.lights, 1zu}), 0zu);
        ::glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, _light_buffer.native_handle());

        ::glProgramUniformHandleui64ARB(_program.native_handle(), 0, scene.the_one_texture.native_handle());
        ::glProgramUniformHandleui64ARB(_program.native_handle(), 1, scene.the_one_normal_map.native_handle());

        ::glMultiDrawElementsIndirect(GL_TRIANGLES,
                                      GL_UNSIGNED_INT,
                                      reinterpret_cast<const void *>(_command_buffer.offset_bytes()),
                                      command_count,
                                      0);

        _command_buffer.advance();
        _camera_buffer.advance();
        _light_buffer.advance();
        _object_data_buffer.advance();
        scene.material_manager.advance();
    }
}
