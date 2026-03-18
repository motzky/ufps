#version 460 core
#extension GL_ARB_bindless_texture : require

const float PI = 3.14159265359;

struct VertexData
{
    float position[3];
    float normal[3];
    float tangent[3];
    float bitangent[3];
    float uv[2];
};

layout(binding = 0, std430) readonly buffer vertices {
    VertexData data[];
};

layout(binding = 1, std430) readonly buffer texture_buffer
{
    sampler2D textures[];
};

layout(location = 0) uniform uint input_tex_index;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 out_color;

void main()
{
    vec3 in_color = texture(textures[input_tex_index], uv).rgb;

    out_color = vec4(in_color.ggg, 1.0);
}
