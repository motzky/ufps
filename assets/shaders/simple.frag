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

layout(binding = 4, std430) readonly buffer lights
{
    float ambient_color[3];
    float point_light_pos[3];
    float point_light_color[3];
    float point_light_attenuation[3];
    float point_light_specular_power;
};

layout(location = 0, bindless_sampler) uniform sampler2D albedo_tex;
layout(location = 1, bindless_sampler) uniform sampler2D normal_tex;
layout(location = 2, bindless_sampler) uniform sampler2D specular_tex;

layout(location = 0) in flat uint material_index;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 frag_position;
layout(location = 3) in mat3 tbn;

layout(location = 0) out vec4 color;

vec3 get_color(uint index)
{
    return vec3(
        material_data[index].color[0],
        material_data[index].color[1],
        material_data[index].color[2]
    );
}

vec3 calc_point(vec3 frag_pos, vec3 view_pos, vec3 n)
{
    vec3 pos = vec3(point_light_pos[0], point_light_pos[1], point_light_pos[2]);
    vec3 color = vec3(point_light_color[0], point_light_color[1], point_light_color[2]);
    vec3 attenuation = vec3(point_light_attenuation[0], point_light_attenuation[1], point_light_attenuation[2]);

    float dist = length(pos - frag_pos);
    float att = 1.0 / (attenuation.x + (attenuation.y * dist) + (attenuation.z * (dist * dist)));

    vec3 light_dir = normalize(pos - frag_pos);
    float diff = max(dot(n, light_dir), 0.0);

    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 half_way = normalize(light_dir + view_dir);
    
    float spec = pow(max(dot(n, half_way), 0.0), point_light_specular_power) * texture(specular_tex, uv).r;
    
    return ((diff + spec) * att) * color;
}

void main()
{
    vec3 nm = texture(normal_tex, uv).xyz;
    nm = (nm*2.0 - 1.0);
    vec3 n = normalize(tbn * nm);

    vec3 albedo = texture(albedo_tex, uv).rgb;
    vec3 ambient_col = vec3(ambient_color[0], ambient_color[1], ambient_color[2]);
    vec3 point_col = calc_point(frag_position.xyz, vec3(camera_position[0],camera_position[1],camera_position[2]), n);

    color = vec4(albedo * (ambient_col + point_col), 1.0);
}

