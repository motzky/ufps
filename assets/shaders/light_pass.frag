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

layout(binding = 0, std430) readonly buffer vertices {
    VertexData data[];
};


layout(binding = 1, std430) readonly buffer texture_buffer
{
    sampler2D textures[];
};

layout(location = 0) uniform uint tex_index;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 color;

void main()
{
    vec3 input_color = texture(textures[tex_index], uv).rgb;

    float grey = (0.299 * input_color.r) + (0.587 * input_color.g) + (0.114 * input_color.b);

    color = vec4(grey, grey, grey, 1.0);
}

