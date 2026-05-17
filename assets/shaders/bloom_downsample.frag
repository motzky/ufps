#version 460 core
#extension GL_ARB_bindless_texture : require

layout(bindless_sampler, location = 0) uniform sampler2D input_texture;
layout(location = 1) uniform vec2 src_resolution;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 out_color;


void main()
{
    vec2 src_texel_size = 1.0 / src_resolution;

    float x = src_texel_size.x;
    float y = src_texel_size.y;

    vec3 a = texture(input_texture, vec2(uv.x - 2*x, uv.y + 2*y)).rgb;
    vec3 b = texture(input_texture, vec2(uv.x,       uv.y + 2*y)).rgb;
    vec3 c = texture(input_texture, vec2(uv.x + 2*x, uv.y + 2*y)).rgb;

    vec3 d = texture(input_texture, vec2(uv.x - 2*x, uv.y)).rgb;
    vec3 e = texture(input_texture, vec2(uv.x,       uv.y)).rgb;
    vec3 f = texture(input_texture, vec2(uv.x + 2*x, uv.y)).rgb;

    vec3 g = texture(input_texture, vec2(uv.x - 2*x, uv.y - 2*y)).rgb;
    vec3 h = texture(input_texture, vec2(uv.x,       uv.y - 2*y)).rgb;
    vec3 i = texture(input_texture, vec2(uv.x + 2*x, uv.y - 2*y)).rgb;

    vec3 j = texture(input_texture, vec2(uv.x - x, uv.y + y)).rgb;
    vec3 k = texture(input_texture, vec2(uv.x + x, uv.y + y)).rgb;
    vec3 l = texture(input_texture, vec2(uv.x - x, uv.y - y)).rgb;
    vec3 m = texture(input_texture, vec2(uv.x + x, uv.y - y)).rgb;

    vec3 color = e*0.125;
    color += (a+c+g+i)*0.03125;
    color += (b+d+f+h)*0.0625;
    color += (j+k+l+m)*0.125;

    out_color = vec4(color, 1.0);
}
