#version 450

layout(push_constant) uniform Push {
    mat4 projection;
    vec4 transform;
    vec4 uvRect;
    vec4 tint;
    int useTexture;
    int invert;
} pc;

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_UvCoords;

layout(location = 0) out vec2 v_UvCoords;

void main()
{
    v_UvCoords = pc.uvRect.xy + a_UvCoords * pc.uvRect.zw;

    vec2 screenPosition = pc.transform.xy + a_Position * pc.transform.zw;
    gl_Position = pc.projection * vec4(screenPosition, 0.0, 1.0);
}
