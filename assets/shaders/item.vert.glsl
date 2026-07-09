#version 450

layout(push_constant) uniform Push {
    mat4 viewProjection;
    vec3 center; // world-space drop centre
    vec3 right;  // camera right, already scaled by size
    vec3 up;     // camera up, already scaled by size
    vec3 tile;   // tile origin in xy, tile size in z
} pc;

layout(location = 0) in vec2 a_Offset; // quad corner, -0.5..0.5
layout(location = 1) in vec2 a_Uv;     // 0..1 across the tile

layout(location = 0) out vec2 v_Uv;

void main()
{
    // Map the corner into the block's atlas tile, then spread the quad across the
    // camera's right/up axes so the icon always faces the viewer like a billboard.
    v_Uv = pc.tile.xy + a_Uv * pc.tile.z;
    vec3 world = pc.center + pc.right * a_Offset.x + pc.up * a_Offset.y;
    gl_Position = pc.viewProjection * vec4(world, 1.0);
}
