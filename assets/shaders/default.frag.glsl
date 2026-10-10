#version 450

layout(push_constant) uniform Push {
    mat4 viewProjection;
    vec3 sunColor;
    float alphaScale;
    vec3 sunDirection;
    float isWater;
    vec3 ambientColor;
} pc;

layout(set = 0, binding = 0) uniform sampler2D u_Texture;

layout(location = 0) in vec2 v_UvCoords;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in float v_SkyLight;
layout(location = 3) in float v_WaterDepth;
layout(location = 4) in vec3 v_Tint;
layout(location = 5) in float v_BlockLight;

layout(location = 0) out vec4 o_Color;

const vec3 k_CaveAmbient = vec3(0.05);

const vec3 k_BlockLightColor = vec3(1.0, 0.8, 0.5);

const float k_WaterMaxDepth = 4.0;
const float k_WaterShallowClarity = 0.55;

void main()
{
    vec4 albedo = texture(u_Texture, v_UvCoords);

    if (pc.isWater < 0.5 && albedo.a < 0.5) {
        discard;
    }

    float diffuse = max(dot(normalize(v_Normal), pc.sunDirection), 0.0);

    vec3 daylight = pc.ambientColor + pc.sunColor * diffuse;
    vec3 skyContribution = v_SkyLight * daylight;
    vec3 blockContribution = v_BlockLight * k_BlockLightColor;
    vec3 light = k_CaveAmbient + max(skyContribution, blockContribution);

    float depthFade = mix(k_WaterShallowClarity, 1.0, clamp(v_WaterDepth / k_WaterMaxDepth, 0.0, 1.0));
    float alpha = albedo.a * pc.alphaScale * mix(1.0, depthFade, pc.isWater);

    o_Color = vec4(albedo.rgb * v_Tint * light, alpha);
}
