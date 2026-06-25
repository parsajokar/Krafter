#version 450 core

layout(location = 0) uniform mat4 u_ViewProjection;
layout(location = 1) uniform vec3 u_BlockPosition; // world-space min corner

layout(location = 0) in vec3 a_Position; // unit cube corner, 0..1

void main()
{
    // Inflate slightly past the block so the lines clear its surface (no z-fighting).
    const float margin = 0.002;
    vec3 world = u_BlockPosition - vec3(margin) + a_Position * (1.0 + 2.0 * margin);
    gl_Position = u_ViewProjection * vec4(world, 1.0);
}
