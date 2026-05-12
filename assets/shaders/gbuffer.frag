#version 460 core
#extension GL_ARB_bindless_texture : require

layout(location = 0) in flat uvec2 albedo_bindless_handle;
layout(location = 1) in flat uvec2 normal_bindless_handle;
layout(location = 2) in flat uvec2 specular_bindless_handle;
layout(location = 3) in flat uvec2 roughness_bindless_handle;
layout(location = 4) in flat uvec2 ao_bindless_handle;
layout(location = 5) in flat uvec2 emissive_bindless_handle;
layout(location = 6) in flat uint normal_compressed;
layout(location = 7) in flat float opacity;
layout(location = 8) in flat float emissive_strength;
layout(location = 9) in vec2 uv;
layout(location = 10) in vec4 frag_position;
layout(location = 11) in mat3 tbn;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_pos;
layout(location = 3) out vec4 out_specular;
layout(location = 4) out vec4 out_roughness;
layout(location = 5) out vec4 out_ao;
layout(location = 6) out vec4 out_emissive_color;

void main()
{
    vec3 nm = vec3(0.0, 0.0, 1.0);
    if(normal_bindless_handle.x < 65535)
    {
        if(normal_compressed != 0)
        {
            nm.xy = texture(sampler2D(normal_bindless_handle), uv).rg * 2.0 - 1.0;
            nm.z = sqrt(max(1.0 - dot(nm.xy, nm.xy), 0.0));
        }
        else
        {
            nm = texture(sampler2D(normal_bindless_handle), uv).xyz;
            nm = (nm*2.0) - 1.0;
        }
    }

    vec3 n = normalize(tbn * nm);

    vec4 albedo = texture(sampler2D(albedo_bindless_handle), uv);

    float o = min(opacity, albedo.a);

    out_color = vec4(albedo.rgb, o);

    out_normal = vec4(n, 1.0);
    out_pos = frag_position;
    if(specular_bindless_handle.x < 65535)
    {
        out_specular = texture(sampler2D(specular_bindless_handle), uv);
    }
    else
    {
        out_specular = vec4(0.0,0.0,0.0,1.0);
    }
    out_roughness = vec4(1.0);
    if(roughness_bindless_handle.x < 65535)
    {
        out_roughness = texture(sampler2D(roughness_bindless_handle), uv);
    }
    out_ao = vec4(1.0);
    if(ao_bindless_handle.x < 65535)
    {
        out_ao = texture(sampler2D(ao_bindless_handle), uv);
    }
    out_emissive_color = vec4(0.0, 0.0, 0.0, 1.0);
    if(emissive_bindless_handle.x < 65535)
    {
        vec4 texel = texture(sampler2D(emissive_bindless_handle), uv);
        out_emissive_color = vec4(texel.rgb * emissive_strength, texel.a);
    }
}
