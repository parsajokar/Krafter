#version 450

layout(push_constant) uniform Push {
    mat4 projection;
    vec4 transform;
    vec4 uvRect;
    vec4 tint;
    int useTexture;
    int invert;
} pc;

layout(set = 0, binding = 0) uniform sampler2D u_Texture;

layout(location = 0) in vec2 v_UvCoords;

layout(location = 0) out vec4 o_Color;

void main()
{
    vec4 color = pc.tint;
    if (pc.useTexture == 1) {
        color *= texture(u_Texture, v_UvCoords);
    }

    // Premultiply so transparent texels are black: with the inverting blend func
    // they leave the background untouched while opaque texels invert it.
    if (pc.invert == 1) {
        color.rgb *= color.a;
    }

    o_Color = color;
}
