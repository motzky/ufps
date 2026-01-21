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

layout(binding = 0, std430) readonly buffer vertices {
    VertexData data[];
};


layout(binding = 1, std430) readonly buffer texture_buffer
{
    sampler2D textures[];
};

layout(binding = 2, std430) readonly buffer lights
{
    float ambient_color[3];
    float point_light_pos[3];
    float point_light_color[3];
    float point_light_attenuation[3];
    float point_light_specular_power;
};

layout(binding = 3, std430) readonly buffer camera
{
    mat4 view;
    mat4 projection;
    float camera_position[3];
};


layout(location = 0) uniform uint albedo_tex_index;
layout(location = 1) uniform uint normal_tex_index;
layout(location = 2) uniform uint position_tex_index;
layout(location = 3) uniform uint specular_tex_index;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 out_color;

void main()
{
    vec3 albedo = texture(textures[albedo_tex_index], uv).rgb;
    vec3 normal = texture(textures[normal_tex_index], uv).xyz;
    vec3 frag_pos = texture(textures[position_tex_index], uv).rgb;
    vec4 specular_tex_value = texture(textures[specular_tex_index], uv);

    float specular = specular_tex_value.r;

    vec3 view_pos = vec3(camera_position[0],camera_position[1],camera_position[2]);

    vec3 point_pos = vec3(point_light_pos[0], point_light_pos[1], point_light_pos[2]);
    vec3 point_color = vec3(point_light_color[0], point_light_color[1], point_light_color[2]);
    vec3 point_attenuation = vec3(point_light_attenuation[0], point_light_attenuation[1], point_light_attenuation[2]);

    vec3 ambient_color = vec3(ambient_color[0], ambient_color[1], ambient_color[2]);
    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 light_dir = normalize(point_pos - frag_pos);


    float dist = length(point_pos - frag_pos);
    float att = 1.0 / (point_attenuation.x + (point_attenuation.y * dist) + (point_attenuation.z * (dist * dist)));

    float diff = max(dot(normal, light_dir), 0.0);

    vec3 half_way = normalize(light_dir + view_dir);
    
    float spec = pow(max(dot(normal, half_way), 0.0), point_light_specular_power) * specular;

    vec3 lighting = (ambient_color * albedo) + (diff + spec) * att * point_color;

    out_color = vec4(lighting, 1.0);
}

