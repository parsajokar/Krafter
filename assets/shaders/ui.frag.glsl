#version 450 core

layout(location = 3) uniform sampler2D u_Texture;
layout(location = 4) uniform vec4 u_Tint;
layout(location = 5) uniform int u_UseTexture;
layout(location = 6) uniform int u_Invert;

layout(location = 0) out vec4 o_Color;

in vec2 v_UvCoords;

void main()
{
    vec4 color = u_Tint;
    if (u_UseTexture == 1) {
        color *= texture(u_Texture, v_UvCoords);
    }

    // Premultiply so transparent texels are black: with the inverting blend func
    // they leave the background untouched while opaque texels invert it.
    if (u_Invert == 1) {
        color.rgb *= color.a;
    }

    o_Color = color;
}
