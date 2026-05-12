#version 460 core
#extension GL_ARB_bindless_texture : require

layout(bindless_sampler, location = 0) uniform sampler2D input_texture;
layout(location = 1) uniform float red_offset;
layout(location = 2) uniform float green_offset;
layout(location = 3) uniform float blue_offset;
layout(location = 4) uniform float strength;
layout(location = 5) uniform vec3 vignette_color;
layout(location = 6) uniform float vignette_strength;
layout(location = 7) uniform float vignette_feather;


layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 out_color;

vec3 chromatic_abberation()
{
    vec2 direction = uv - vec2(0.5, 0.5);
    float dist = length(direction);
    float mask = smoothstep(strength, strength + 0.2, dist);
    vec2 shift = direction * mask;

    float red = texture(input_texture, uv + (shift * red_offset)).r;
    float green = texture(input_texture, uv  + (shift * green_offset)).g;
    float blue = texture(input_texture, uv + (shift * blue_offset)).b;

    return vec3(red, green, blue);
}

vec3 vignette(vec3 color)
{
    vec2 direction = uv - vec2(0.5, 0.5);
    float dist = length(direction);
    float vignette_amount = smoothstep(vignette_strength, vignette_strength + vignette_feather, dist);
    return mix(color, vignette_color, vignette_amount);
}

void main()
{
    vec3 color = chromatic_abberation();
    color = vignette(color);

    vec4 in_color = texture(input_texture, uv);
    out_color = vec4(color, in_color.a);
}