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

struct ObjectData
{
    mat4 model;
    uint material_index;
};

struct MaterialData
{
    float color[3];
    uint albedo_index;
    uint normal_index;
    uint specular_index;
    uint roughness_index;
    uint ao_index;
    uint emissive_index;
};

layout(binding = 0, std430) readonly buffer vertices {
    VertexData data[];
};

layout(binding = 1, std430) readonly buffer camera {
    mat4 view;
    mat4 projection;
    float camera_position[3];
};

layout(binding = 2, std430) readonly buffer objects {
    ObjectData object_data[];
};

layout(binding = 3, std430) readonly buffer materials
{
    MaterialData material_data[];
};

layout(binding = 4, std430) readonly buffer texutres
{
    sampler2D textures[];
};

layout(location = 0) in flat uint material_index;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 frag_position;
layout(location = 3) in mat3 tbn;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_pos;
layout(location = 3) out vec4 out_specular;
layout(location = 4) out vec4 out_roughness;
layout(location = 5) out vec4 out_ao;
layout(location = 6) out vec4 out_emissive_color;

vec3 get_color(uint index)
{
    return vec3(
        material_data[index].color[0],
        material_data[index].color[1],
        material_data[index].color[2]
    );
}

void main()
{
    uint albedo_tex_index = material_data[material_index].albedo_index;
    uint normal_tex_index = material_data[material_index].normal_index;
    uint specular_tex_index = material_data[material_index].specular_index;
    uint roughness_tex_index = material_data[material_index].roughness_index;
    uint ao_tex_index = material_data[material_index].ao_index;
    uint emissive_tex_index = material_data[material_index].emissive_index;

    vec3 n = vec3(1.0);
    if(normal_tex_index < 65535)
    {
        vec3 nm = texture(textures[normal_tex_index], uv).xyz;
        nm = (nm*2.0 - 1.0);
        
        n = normalize(tbn * nm);
    }

    out_color = texture(textures[albedo_tex_index], uv);
    out_normal = vec4(n, 1.0);
    out_pos = frag_position;
    if(specular_tex_index < 65535)
    {
        out_specular = texture(textures[specular_tex_index], uv);
    }
    else
    {
        out_specular = vec4(1.0);
    }
    out_roughness = vec4(1.0);
    if(roughness_tex_index < 65535)
    {
        out_roughness = texture(textures[roughness_tex_index], uv);
    }
    out_ao = vec4(1.0);
    if(ao_tex_index < 65535)
    {
        out_ao = texture(textures[ao_tex_index], uv);
    }
    out_emissive_color = vec4(0.0, 0.0, 0.0, 1.0);
    if(emissive_tex_index < 65535)
    {
        out_emissive_color = texture(textures[emissive_tex_index], uv);
    }
}

