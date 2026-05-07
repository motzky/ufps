#version 460 core
#extension GL_ARB_bindless_texture : require

layout(binding = 1, std430) readonly buffer camera {
    mat4 view;
    mat4 projection;
    float camera_position[3];
};

layout(binding = 2, std430) readonly buffer ssao_samples{
    vec4 samples[];
};

layout(bindless_sampler, location = 0) uniform sampler2D normal_texture;
layout(bindless_sampler, location = 1) uniform sampler2D position_texture;
layout(bindless_sampler, location = 2) uniform sampler2D ao_texture;
layout(bindless_sampler, location = 3) uniform sampler2D emissive_texture;
layout(location = 4) uniform float width;
layout(location = 5) uniform float height;
layout(location = 6) uniform uint sample_count;
layout(location = 7) uniform float radius;
layout(location = 8) uniform float bias;
layout(location = 9) uniform float power;
layout(bindless_sampler, location = 10) uniform sampler2D noise_texture;

layout(location = 0) in vec2 in_uv;

out vec4 frag_color;

void main()
{
    const vec3 emissive = texture(emissive_texture, in_uv).xyz;
    if(length(emissive) > 0.0)
    {
        frag_color = vec4(1.0);
        return;
    }

    // const vec2 uv = vec2(in_uv.x, 1.0 - in_uv.y);
    const vec2 uv = vec2(in_uv.x, in_uv.y);

    const vec3 normal = normalize((view * vec4(texture(normal_texture, in_uv).xyz, 0.0)).xyz);
    const vec3 frag_pos = (view * vec4(texture(position_texture, in_uv).xyz, 1.0)).xyz;

    const vec2 size = vec2(width, height);

    const int x = int(uv.x * size.x) % 4;
    const int y = int(uv.y * size.y) % 4;
    const int index = (y * 4) + x;
    const vec2 noise_scale = vec2(width / 4.0, height / 4.0);
    const vec3 rand = normalize(texture(noise_texture, noise_scale).xyz);

    const vec3 tangent = normalize(rand - normal * dot(rand, normal));
    const vec3 bitangent = cross(normal, tangent);
    const mat3 tbn = mat3(tangent, bitangent, normal);

    float baked_occlusion = texture(ao_texture, in_uv).r;

    float occlusion = 1.0;
    for(int i = 0; i < sample_count; ++i)
    {
        vec3 sample_pos = tbn * samples[i].xyz;
        sample_pos = frag_pos + sample_pos * radius;

        vec4 offset = projection * vec4(sample_pos, 1.0);
        offset.xyz /= offset.w;
        offset.xzy = offset.xzy * 0.5 + 0.5;

        const float sample_depth = (view * vec4(texture(position_texture, offset.xy).xyz, 1.0)).z;
        const float range_check = smoothstep(0.0, 1.0, radius / abs(frag_pos.z - sample_depth));
        occlusion += (sample_depth >= sample_pos.z + bias ? 1.0 : 0.0) * range_check;
    }

    occlusion = 1.0 - (occlusion / sample_count);
    occlusion = pow(occlusion, power);

    occlusion = baked_occlusion * occlusion;


    frag_color = vec4(occlusion, 0.0, 0.0, 1.0);
}
