#version 460 core
#extension GL_ARB_bindless_texture : require

layout(bindless_sampler, location = 0) uniform sampler2D input_texture;
layout(location = 1) uniform float filter_radius;

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

    vec3 color = e*4.0;
    color += (b+d+f+h)*2.0;
    color += (a+c+g+i);
    color *= 1.0 / 16.0;

    out_color = vec4(color, 1.0);

}
