#version 450

layout(push_constant) uniform Push {
    mat4 viewProjection;
    vec3 blockPosition; // world-space min corner
    float frameBase;    // bottom v of the active crack frame
    float frameSpan;    // height of one crack frame in v
} pc;

layout(location = 0) in vec3 a_Position; // unit cube corner, 0..1
layout(location = 1) in vec2 a_Uv;       // face-local 0..1

layout(location = 0) out vec2 v_Uv;

void main()
{
    // The crack strip is a vertical stack of frames; map the face's v into the
    // band of the stage being shown so every face wears the same crack.
    v_Uv = vec2(a_Uv.x, pc.frameBase + a_Uv.y * pc.frameSpan);
    gl_Position = pc.viewProjection * vec4(pc.blockPosition + a_Position, 1.0);
}
