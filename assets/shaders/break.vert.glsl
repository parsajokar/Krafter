#version 450 core

layout(location = 0) uniform mat4 u_ViewProjection;
layout(location = 1) uniform vec3 u_BlockPosition; // world-space min corner
layout(location = 2) uniform float u_FrameBase;    // bottom v of the active crack frame
layout(location = 3) uniform float u_FrameSpan;    // height of one crack frame in v

layout(location = 0) in vec3 a_Position; // unit cube corner, 0..1
layout(location = 1) in vec2 a_Uv;       // face-local 0..1

out vec2 v_Uv;

void main()
{
    // The crack strip is a vertical stack of frames; map the face's v into the
    // band of the stage being shown so every face wears the same crack.
    v_Uv = vec2(a_Uv.x, u_FrameBase + a_Uv.y * u_FrameSpan);
    gl_Position = u_ViewProjection * vec4(u_BlockPosition + a_Position, 1.0);
}
