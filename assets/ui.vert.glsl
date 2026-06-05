#version 450 core

layout(location = 0) uniform mat4 u_Projection;
layout(location = 1) uniform vec4 u_Transform; // xy = top-left position (pixels), zw = size (pixels)
layout(location = 2) uniform vec4 u_UvRect;    // xy = uv min, zw = uv size

layout(location = 0) in vec2 a_Position; // unit quad, 0..1
layout(location = 1) in vec2 a_UvCoords; // unit quad, 0..1

out vec2 v_UvCoords;

void main()
{
    v_UvCoords = u_UvRect.xy + a_UvCoords * u_UvRect.zw;

    vec2 screenPosition = u_Transform.xy + a_Position * u_Transform.zw;
    gl_Position = u_Projection * vec4(screenPosition, 0.0, 1.0);
}
