#version 450 core

layout(location = 4) uniform sampler2D u_Texture;

in vec2 v_Uv;

layout(location = 0) out vec4 o_Color;

void main()
{
    // Dark cracks on a transparent ground: drop the clear texels so only the
    // crack lines blend over the block's own face.
    vec4 crack = texture(u_Texture, v_Uv);
    if (crack.a < 0.01) {
        discard;
    }
    o_Color = crack;
}
