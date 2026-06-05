#version 450 core

layout(location = 0) uniform mat4 u_ViewProjection;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_UvCoords;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in float a_SkyLight;

out vec2 v_UvCoords;
out vec3 v_Normal;
out float v_SkyLight;

void main()
{
    v_UvCoords = a_UvCoords;
    v_Normal = a_Normal;
    v_SkyLight = a_SkyLight;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}
