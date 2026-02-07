#version 460 core
#extension GL_ARB_bindless_texture : require

const float PI = 3.14159265359;

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
layout(location = 3) uniform uint metallic_tex_index;
layout(location = 4) uniform uint roughness_tex_index;
layout(location = 5) uniform uint ao_tex_index;
layout(location = 6) uniform uint emissive_tex_index;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 out_color;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N,V), 0.0);
    float NdotL = max(dot(N,L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

void main()
{
    vec3 albedo = pow(texture(textures[albedo_tex_index], uv).rgb, vec3(2.2));
    vec3 normal = texture(textures[normal_tex_index], uv).xyz;
    vec3 frag_pos = texture(textures[position_tex_index], uv).rgb;
    vec3 metallic_tex = texture(textures[metallic_tex_index], uv).rgb;
    vec3 emissive = texture(textures[emissive_tex_index], uv).rgb;

    float emissiveness = length(emissive);

    vec3 Lo = vec3(0.0);
    if(emissiveness > 0.001)
    {
        out_color = vec4(emissive, 1.0);
        return;
    }

    float metallic = metallic_tex.r;
    float roughness = texture(textures[roughness_tex_index], uv).r;
    float ao = texture(textures[ao_tex_index], uv).r;

    vec3 view_pos = vec3(camera_position[0],camera_position[1],camera_position[2]);

    vec3 point_pos = vec3(point_light_pos[0], point_light_pos[1], point_light_pos[2]);
    vec3 point_color = vec3(point_light_color[0], point_light_color[1], point_light_color[2]);
    vec3 point_attenuation = vec3(point_light_attenuation[0], point_light_attenuation[1], point_light_attenuation[2]);

    vec3 ambient_color = vec3(ambient_color[0], ambient_color[1], ambient_color[2]);
    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 light_dir = normalize(point_pos - frag_pos);
    vec3 half_way = normalize(light_dir + view_dir);

    float dist = length(point_pos - frag_pos);
    float att = 1.0 / (point_attenuation.x + (point_attenuation.y * dist) + (point_attenuation.z * (dist * dist)));
    vec3 radiance = point_color * att;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    float NDF = DistributionGGX(normal, half_way, roughness);
    float G = GeometrySmith(normal, view_dir, light_dir, roughness);
    vec3 F = fresnelSchlick(max(dot(view_dir, half_way), 0.0), F0);

    vec3 num = NDF * G * F;
    float denom = 4.0 * max(dot(normal, view_dir), 0.0) * max(dot(normal, light_dir), 0.0) + 0.0001;

    vec3 specular = num / denom;

    vec3 ks = F;
    vec3 kD = vec3(1.0) - ks;

    kD *= 1.0 - metallic;

    float NdotL = max(dot(normal, light_dir), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    out_color = vec4(color, 1.0);
}
