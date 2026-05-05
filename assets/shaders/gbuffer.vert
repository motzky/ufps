#version 460 core

struct VertexData
{
    float position[3];
    float normal[3];
    float tangent[3];
    float bitangent[3];
    float uv[2];
};

struct ObjectData
{
    mat4 model;
    uint albedo_tex_index;
    uint normal_tex_index;
    uint specular_tex_index;
    uint roughness_tex_index;
    uint ao_tex_index;
    uint emissive_tex_index;
    uint normal_compressed;
    uint padding;
    float opacity;

};

layout(binding = 0, std430) readonly buffer vertices
{
    VertexData data[];
};

layout(binding = 1, std430) readonly buffer camera
{
    mat4 view;
    mat4 projection;
    float camera_position[3];
};

layout(binding = 2, std430) readonly buffer objects
{
    ObjectData object_data[];
};

vec3 get_position(uint index)
{
    return vec3(
        data[index].position[0],
        data[index].position[1],
        data[index].position[2]
    );
}
vec3 get_normal(uint index)
{
    return vec3(
        data[index].normal[0],
        data[index].normal[1],
        data[index].normal[2]
    );
}
vec3 get_tangent(uint index)
{
    return vec3(
        data[index].tangent[0],
        data[index].tangent[1],
        data[index].tangent[2]
    );
}
vec3 get_bitangent(uint index)
{
    return vec3(
        data[index].bitangent[0],
        data[index].bitangent[1],
        data[index].bitangent[2]
    );
}
vec2 get_uv(uint index)
{
    return vec2(data[index].uv[0],
                data[index].uv[1]);
}

layout(location = 0) out flat uint albedo_tex_index;
layout(location = 1) out flat uint normal_tex_index;
layout(location = 2) out flat uint specular_tex_index;
layout(location = 3) out flat uint roughness_tex_index;
layout(location = 4) out flat uint ao_tex_index;
layout(location = 5) out flat uint emissive_tex_index;
layout(location = 6) out flat uint normal_compressed;
layout(location = 7) out flat float opacity;
layout(location = 8) out vec2 uv;
layout(location = 9) out vec4 frag_position;
layout(location = 10) out mat3 tbn;

void main()
{
    mat3 normal_mat = transpose(inverse(mat3(object_data[gl_DrawID].model)));

    frag_position = object_data[gl_DrawID].model * vec4(get_position(gl_VertexID), 1.0);
    gl_Position = projection * view * frag_position;
    albedo_tex_index = object_data[gl_DrawID].albedo_tex_index;
    normal_tex_index = object_data[gl_DrawID].normal_tex_index;
    specular_tex_index = object_data[gl_DrawID].specular_tex_index;
    roughness_tex_index = object_data[gl_DrawID].roughness_tex_index;
    ao_tex_index = object_data[gl_DrawID].ao_tex_index;
    emissive_tex_index = object_data[gl_DrawID].emissive_tex_index;
    normal_compressed = object_data[gl_DrawID].normal_compressed;
    opacity = object_data[gl_DrawID].opacity;
    uv = get_uv(gl_VertexID);

    vec3 t = normalize(normal_mat * get_tangent(gl_VertexID));
    vec3 b = normalize(normal_mat * get_bitangent(gl_VertexID));
    vec3 n = normalize(normal_mat * get_normal(gl_VertexID));

    tbn = mat3(t,b,n);
}
