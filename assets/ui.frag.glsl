#version 450 core

layout(location = 3) uniform sampler2D u_Texture;
layout(location = 4) uniform vec4 u_Tint;
layout(location = 5) uniform int u_UseTexture;

layout(location = 0) out vec4 o_Color;

in vec2 v_UvCoords;

void main()
{
    vec4 color = u_Tint;
    if (u_UseTexture == 1) {
        color *= texture(u_Texture, v_UvCoords);
    }
    o_Color = color;
}
