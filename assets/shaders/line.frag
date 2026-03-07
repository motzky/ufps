#version 460 core

struct LineData
{
    float position[3];
    float color[3];
};

layout(binding = 0, std430) readonly buffer lines {
    LineData data[];
};

layout(binding = 1, std430) readonly buffer camera {
    mat4 view;
    mat4 projection;
    float camera_position[3];
    float pad;
};

layout(location = 0) in vec3 in_color;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(in_color, 1.0);
}
