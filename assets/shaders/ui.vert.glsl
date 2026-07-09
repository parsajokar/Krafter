#version 450

layout(push_constant) uniform Push {
    mat4 projection;
    vec4 transform; // xy = top-left position (pixels), zw = size (pixels)
    vec4 uvRect;    // xy = uv min, zw = uv size
    vec4 tint;
    int useTexture;
    int invert;
} pc;

layout(location = 0) in vec2 a_Position; // unit quad, 0..1
layout(location = 1) in vec2 a_UvCoords; // unit quad, 0..1

layout(location = 0) out vec2 v_UvCoords;

void main()
{
    v_UvCoords = pc.uvRect.xy + a_UvCoords * pc.uvRect.zw;

    vec2 screenPosition = pc.transform.xy + a_Position * pc.transform.zw;
    gl_Position = pc.projection * vec4(screenPosition, 0.0, 1.0);
}
