#version 450 core

layout(location = 1) uniform sampler2D u_Texture;
layout(location = 2) uniform vec3 u_SunColor;
layout(location = 3) uniform vec3 u_SunDirection;
layout(location = 4) uniform vec3 u_AmbientColor;

layout(location = 0) out vec4 o_Color;

in vec2 v_UvCoords;
in vec3 v_Normal;
in float v_SkyLight;

// Floor so fully enclosed spaces are dark but not pure black.
const vec3 k_CaveAmbient = vec3(0.05);

void main()
{
    vec4 albedo = texture(u_Texture, v_UvCoords);

    float diffuse = max(dot(normalize(v_Normal), u_SunDirection), 0.0);

    // Sky light gates how much daylight reaches the face, so sealed spaces go
    // dark while openings let the sky and sun in.
    vec3 daylight = u_AmbientColor + u_SunColor * diffuse;
    vec3 light = k_CaveAmbient + v_SkyLight * daylight;

    o_Color = vec4(albedo.rgb * light, albedo.a);
}
