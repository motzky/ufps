#version 460 core

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

layout(binding = 3, std430) readonly buffer camera
{
    mat4 view;
    mat4 projection;
    float camera_position[3];
};

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform vec3 color;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(color, 1.0);
}