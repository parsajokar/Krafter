#version 450

// Per-draw constants. The vertex stage only reads viewProjection, but the block
// is declared identically in both stages so the push-constant layout matches.
layout(push_constant) uniform Push {
    mat4 viewProjection;
    vec3 sunColor;
    float alphaScale;
    vec3 sunDirection;
    float isWater;
    vec3 ambientColor;
} pc;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_UvCoords;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in float a_SkyLight;
layout(location = 4) in float a_WaterDepth;
layout(location = 5) in vec3 a_Tint;
layout(location = 6) in float a_BlockLight;

layout(location = 0) out vec2 v_UvCoords;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out float v_SkyLight;
layout(location = 3) out float v_WaterDepth;
layout(location = 4) out vec3 v_Tint;
layout(location = 5) out float v_BlockLight;

void main()
{
    v_UvCoords = a_UvCoords;
    v_Normal = a_Normal;
    v_SkyLight = a_SkyLight;
    v_WaterDepth = a_WaterDepth;
    v_Tint = a_Tint;
    v_BlockLight = a_BlockLight;
    gl_Position = pc.viewProjection * vec4(a_Position, 1.0);
}
