#version 460 core
#extension GL_ARB_bindless_texture : require

layout(binding = 1, std430) readonly buffer average_buffer
{
    float average;
};

layout(binding = 2, std430) readonly buffer camera {
    mat4 view;
    mat4 projection;
    float camera_position[3];
};

layout(bindless_sampler, location = 0) uniform sampler2D input_texture;
layout(location = 1) uniform float in_P;
layout(location = 2) uniform float in_a;
layout(location = 3) uniform float in_m;
layout(location = 4) uniform float in_l;
layout(location = 5) uniform float in_c;
layout(location = 6) uniform float in_b;
layout(location = 7) uniform float in_gamma;
layout(bindless_sampler, location = 8) uniform sampler2D ssao_texture;
layout(bindless_sampler, location = 9) uniform sampler2D depth_texture;
layout(location = 10) uniform vec3 fog_color;
layout(location = 11) uniform float fog_density;


layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 out_color;

vec3 uchimura(vec3 x, float P, float a, float m, float l, float c, float b)
{
    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    vec3 w0 = vec3(1.0 - smoothstep(0.0, m, x));
    vec3 w2 = vec3(step(m + l0, x));
    vec3 w1 = vec3(1.0 - w0 - w2);

    vec3 T = vec3(m * pow(x / m, vec3(c)) + b);
    vec3 S = vec3(P - (P - S1) * exp(CP * (x - S0)));
    vec3 L = vec3(m + a * (x - m));

    return T * w0 + L * w1 + S * w2;
}

vec3 convertRGB2XYZ(vec3 _rgb)
{
	// Reference:
	// RGB/XYZ Matrices
	// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
	vec3 xyz;
	xyz.x = dot(vec3(0.4124564, 0.3575761, 0.1804375), _rgb);
	xyz.y = dot(vec3(0.2126729, 0.7151522, 0.0721750), _rgb); 
	xyz.z = dot(vec3(0.0193339, 0.1191920, 0.9503041), _rgb);
	return xyz;
}

vec3 convertXYZ2Yxy(vec3 _xyz)
{
	// Reference:
	// http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_xyY.html
	float inv = 1.0/dot(_xyz, vec3(1.0, 1.0, 1.0) );
	return vec3(_xyz.y, _xyz.x*inv, _xyz.y*inv);
}

vec3 convertRGB2Yxy(vec3 _rgb)
{
	return convertXYZ2Yxy(convertRGB2XYZ(_rgb) );
}

vec3 convertYxy2XYZ(vec3 _Yxy)
{
	// Reference:
	// http://www.brucelindbloom.com/index.html?Eqn_xyY_to_XYZ.html
	vec3 xyz;
	xyz.x = _Yxy.x*_Yxy.y/_Yxy.z;
	xyz.y = _Yxy.x;
	xyz.z = _Yxy.x*(1.0 - _Yxy.y - _Yxy.z)/_Yxy.z;
	return xyz;
}

vec3 convertXYZ2RGB(vec3 _xyz)
{
	vec3 rgb;
	rgb.x = dot(vec3( 3.2404542, -1.5371385, -0.4985314), _xyz);
	rgb.y = dot(vec3(-0.9692660,  1.8760108,  0.0415560), _xyz);
	rgb.z = dot(vec3( 0.0556434, -0.2040259,  1.0572252), _xyz);
	return rgb;
}

vec3 convertYxy2RGB(vec3 _Yxy)
{
	return convertXYZ2RGB(convertYxy2XYZ(_Yxy) );
}

vec3 fog(float depth, vec3 color)
{
    float fog_amount = 1.0/exp((depth * fog_density) * (depth * fog_density));
    return mix(fog_color, color, fog_amount);
}

void main()
{
    vec4 input_color = texture(input_texture, uv);
    vec3 col = input_color.rgb;
    
    vec3 eye = vec3(camera_position[0],camera_position[1],camera_position[2]);
    vec3 frag_pos = texture(depth_texture, uv).xyz;
    float depth = length(frag_pos - eye);

    vec3 Yxz = convertRGB2Yxy(col);

    Yxz.x /= (9.6 * average + 0.0001);
    float occlusion = texture(ssao_texture, uv).r;

    vec3 in_color = convertYxy2RGB(Yxz);
    in_color = fog(depth, in_color * occlusion);

    vec3 tone_mapped_color = uchimura(in_color, in_P, in_a, in_m, in_l, in_c, in_b);
    

    vec3 gamma_corrected = pow(tone_mapped_color, vec3(1.0 / in_gamma));

    out_color = vec4(gamma_corrected, input_color.a);
}
