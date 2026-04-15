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

layout(location = 0) uniform uint ssao_tex_index;
layout(location = 1) uniform uint depth_tex_index;
layout(location = 2) uniform float width;
layout(location = 3) uniform float height;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 frag_color;

void main()
{
    vec2 texel_size = 1.0 / vec2(width, height);

    float center_depth = texture(textures[depth_tex_index], in_uv).r;

    float result = 0.0;
    float total_weight = 0.0;

    for(int x = -2; x < 2; ++x)
    {
        for(int y = -2; y < 2; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texel_size;

            float sample_ao = texture(textures[ssao_tex_index], in_uv + offset).r;
            float sample_depth = texture(textures[depth_tex_index], in_uv + offset).r;

            float depth_diff = abs(center_depth - sample_depth);
            float weight = step(depth_diff, 0.05);

            result += sample_ao * weight;
            total_weight += weight; 
        }
    }

    float final_ao = result / (total_weight + 0.0001);

    frag_color = vec4(final_ao, 0.0, 0.0, 1.0);
}
