#version 450 core

layout(location = 0) uniform mat4 u_ViewProjection;
layout(location = 1) uniform vec3 u_Center; // world-space drop centre
layout(location = 2) uniform vec3 u_Right;  // camera right, already scaled by size
layout(location = 3) uniform vec3 u_Up;     // camera up, already scaled by size
layout(location = 4) uniform vec3 u_Tile;   // tile origin in xy, tile size in z

layout(location = 0) in vec2 a_Offset; // quad corner, -0.5..0.5
layout(location = 1) in vec2 a_Uv;     // 0..1 across the tile

out vec2 v_Uv;

void main()
{
    // Map the corner into the block's atlas tile, then spread the quad across the
    // camera's right/up axes so the icon always faces the viewer like a billboard.
    v_Uv = u_Tile.xy + a_Uv * u_Tile.z;
    vec3 world = u_Center + u_Right * a_Offset.x + u_Up * a_Offset.y;
    gl_Position = u_ViewProjection * vec4(world, 1.0);
}
