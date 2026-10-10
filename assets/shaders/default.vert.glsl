#version 450

layout(push_constant) uniform Push {
    mat4 viewProjection;
    vec3 sunColor;
    float alphaScale;
    vec3 sunDirection;
    float isWater;
    vec3 ambientColor;
} pc;

layout(location = 0) in uvec4 a_PositionDepth;
layout(location = 1) in vec2 a_UvCoords;
layout(location = 2) in vec4 a_NormalBlockLight;
layout(location = 3) in vec4 a_TintSkyLight;

layout(location = 4) in vec3 i_ChunkOrigin;

const float k_PositionScale = 128.0;

layout(location = 0) out vec2 v_UvCoords;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out float v_SkyLight;
layout(location = 3) out float v_WaterDepth;
layout(location = 4) out vec3 v_Tint;
layout(location = 5) out float v_BlockLight;

void main()
{
    v_UvCoords = a_UvCoords;
    v_Normal = a_NormalBlockLight.xyz;
    v_SkyLight = a_TintSkyLight.a;
    v_WaterDepth = float(a_PositionDepth.w);
    v_Tint = a_TintSkyLight.rgb;
    v_BlockLight = a_NormalBlockLight.w;
    vec3 position = i_ChunkOrigin + vec3(a_PositionDepth.xyz) / k_PositionScale;
    gl_Position = pc.viewProjection * vec4(position, 1.0);
}
