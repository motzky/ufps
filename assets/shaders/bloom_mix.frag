#version 460 core
#extension GL_ARB_bindless_texture : require

layout(bindless_sampler, location = 0) uniform sampler2D input_texture;
layout(bindless_sampler, location = 1) uniform sampler2D light_pass_texture;
layout(location = 2) uniform float filter_radius;
layout(location = 3) uniform float mix_amount;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 out_color;


void main()
{
    float x = filter_radius;
    float y = filter_radius;

    vec3 a = texture(input_texture, vec2(uv.x - x, uv.y + y)).rgb;
    vec3 b = texture(input_texture, vec2(uv.x,     uv.y + y)).rgb;
    vec3 c = texture(input_texture, vec2(uv.x + x, uv.y + y)).rgb;

    vec3 d = texture(input_texture, vec2(uv.x - x, uv.y)).rgb;
    vec3 e = texture(input_texture, vec2(uv.x,     uv.y)).rgb;
    vec3 f = texture(input_texture, vec2(uv.x + x, uv.y)).rgb;

    vec3 g = texture(input_texture, vec2(uv.x - x, uv.y - y)).rgb;
    vec3 h = texture(input_texture, vec2(uv.x,     uv.y - y)).rgb;
    vec3 i = texture(input_texture, vec2(uv.x + x, uv.y - y)).rgb;

    vec3 final_mip_color =  e*4.0;
    final_mip_color += (b+d+f+h)*2.0;
    final_mip_color += (a+c+g+i);
    final_mip_color *= 1.0 / 16.0;

    vec3 light_pass_color = texture(light_pass_texture, uv).rgb;

    vec3 mixed = mix(light_pass_color, final_mip_color, mix_amount);

    out_color = vec4(mixed, 1.0);
}
