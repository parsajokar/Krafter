#version 450

layout(push_constant) uniform Push {
    mat4 viewProjection;
    vec3 center;
    vec3 right;
    vec3 up;
    vec3 tile;
} pc;

layout(location = 0) in vec2 a_Offset;
layout(location = 1) in vec2 a_Uv;

layout(location = 0) out vec2 v_Uv;

void main()
{
    v_Uv = pc.tile.xy + a_Uv * pc.tile.z;
    vec3 world = pc.center + pc.right * a_Offset.x + pc.up * a_Offset.y;
    gl_Position = pc.viewProjection * vec4(world, 1.0);
}
