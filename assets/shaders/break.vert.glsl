#version 450

layout(push_constant) uniform Push {
    mat4 viewProjection;
    vec3 blockPosition;
    float frameBase;
    float frameSpan;
} pc;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_Uv;

layout(location = 0) out vec2 v_Uv;

void main()
{
    v_Uv = vec2(a_Uv.x, pc.frameBase + a_Uv.y * pc.frameSpan);
    gl_Position = pc.viewProjection * vec4(pc.blockPosition + a_Position, 1.0);
}
