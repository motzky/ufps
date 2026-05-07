#version 460 core
#extension GL_ARB_bindless_texture : require

const float PI = 3.14159265359;

struct PointLight
{
    float pos[3];
    float color[3];
    float attenuation[3];
    float specular_power;
    float intensity;
    float pad;
};

layout(binding = 1, std430) readonly buffer lights
{
    float ambient_color[3];
    uint num_point_lights;
    PointLight point_lights[];
};

layout(binding = 2, std430) readonly buffer camera
{
    mat4 view;
    mat4 projection;
    float camera_position[3];
};


layout(bindless_sampler, location = 0) uniform sampler2D albedo_texture;
layout(bindless_sampler, location = 1) uniform sampler2D normal_texture;
layout(bindless_sampler, location = 2) uniform sampler2D position_texture;
layout(bindless_sampler, location = 3) uniform sampler2D metallic_texture;
layout(bindless_sampler, location = 4) uniform sampler2D roughness_texture;
layout(bindless_sampler, location = 5) uniform sampler2D ao_texture;
layout(bindless_sampler, location = 6) uniform sampler2D emissive_texture;

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

vec3 calculate_point_light(PointLight light, vec3 view_pos, vec3 view_dir, vec3 normal, vec3 frag_pos, vec3 albedo, vec3 F0, float metallic, float roughness, float ao)
{
    vec3 point_pos = vec3(light.pos[0], light.pos[1], light.pos[2]);
    vec3 point_color = vec3(light.color[0], light.color[1], light.color[2]) * vec3(light.intensity);
    vec3 point_attenuation = vec3(light.attenuation[0], light.attenuation[1], light.attenuation[2]);

    vec3 light_dir = normalize(point_pos - frag_pos);
    vec3 half_way = normalize(light_dir + view_dir);

    float dist = length(point_pos - frag_pos);
    float att = 1.0 / (point_attenuation.x + (point_attenuation.y * dist) + (point_attenuation.z * (dist * dist)));
    vec3 radiance = point_color * att;

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
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main()
{
    vec4 albedo_texel = texture(albedo_texture, uv);
    vec3 albedo = albedo_texel.rgb;
    float alpha = albedo_texel.a;    
    vec3 normal = texture(normal_texture, uv).xyz;
    vec3 frag_pos = texture(position_texture, uv).rgb;
    vec3 metallic_tex = texture(metallic_texture, uv).rgb;
    vec4 emissive_texel = texture(emissive_texture, uv);
    vec3 emissive = emissive_texel.rgb;

    float emissiveness = length(emissive);

    if(emissiveness > 0.001)
    {
        out_color = vec4(emissive, emissive_texel.a);
        return;
    }

    vec3 ambient_color = vec3(ambient_color[0], ambient_color[1], ambient_color[2]);

    float metallic = metallic_tex.r;
    float roughness = texture(roughness_texture, uv).r;
    float ao = texture(ao_texture, uv).r;

    vec3 view_pos = vec3(camera_position[0],camera_position[1],camera_position[2]);
    vec3 view_dir = normalize(view_pos - frag_pos);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    for(uint i = 0; i < num_point_lights; i++)
    {
        Lo += calculate_point_light(point_lights[i], view_pos, view_dir, normal, frag_pos, albedo, F0, metallic, roughness, ao);
    }

    vec3 ambient = ambient_color * albedo * ao;
    vec3 color = ambient + Lo;

    out_color = vec4(color, alpha);
}
