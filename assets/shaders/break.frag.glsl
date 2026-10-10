#version 450

layout(set = 0, binding = 0) uniform sampler2D u_Texture;

layout(location = 0) in vec2 v_Uv;

layout(location = 0) out vec4 o_Color;

void main()
{
    vec4 crack = texture(u_Texture, v_Uv);
    if (crack.a < 0.01) {
        discard;
    }
    o_Color = crack;
}
