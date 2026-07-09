#version 450

layout(push_constant) uniform Push {
    mat4 viewProjection;
    vec3 sunColor;
    float alphaScale;
    vec3 sunDirection;
    float isWater;
    vec3 ambientColor;
} pc;

layout(set = 0, binding = 0) uniform sampler2D u_Texture;

layout(location = 0) in vec2 v_UvCoords;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in float v_SkyLight;
layout(location = 3) in float v_WaterDepth;
layout(location = 4) in vec3 v_Tint;
layout(location = 5) in float v_BlockLight;

layout(location = 0) out vec4 o_Color;

// Floor so fully enclosed spaces are dark but not pure black.
const vec3 k_CaveAmbient = vec3(0.05);

// Warm colour of emitted block light (lava, torches) at full strength; unlike
// sky light it does not dim with the day/night cycle.
const vec3 k_BlockLightColor = vec3(1.0, 0.8, 0.5);

// Water fades from clear in the shallows to fully tinted at this depth (blocks).
const float k_WaterMaxDepth = 8.0;
const float k_WaterShallowClarity = 0.2;

void main()
{
    vec4 albedo = texture(u_Texture, v_UvCoords);

    // Cutout transparency for foliage (leaves, cactus edges): drop fully clear
    // texels so they neither shade nor write depth. Water is excluded because its
    // translucency comes from the shader, not the texture's alpha.
    if (pc.isWater < 0.5 && albedo.a < 0.5) {
        discard;
    }

    float diffuse = max(dot(normalize(v_Normal), pc.sunDirection), 0.0);

    // Sky light gates how much daylight reaches the face, so sealed spaces go
    // dark while openings let the sky and sun in. Block light adds a warm glow
    // that survives the dark; the brighter of the two channels wins per component
    // so a torch-lit cave and a sunlit field don't stack into a blown-out white.
    vec3 daylight = pc.ambientColor + pc.sunColor * diffuse;
    vec3 skyContribution = v_SkyLight * daylight;
    vec3 blockContribution = v_BlockLight * k_BlockLightColor;
    vec3 light = k_CaveAmbient + max(skyContribution, blockContribution);

    // Deeper water hides the bottom; shallow water stays clear. Opaque geometry
    // (isWater = 0) keeps its full alpha.
    float depthFade = mix(k_WaterShallowClarity, 1.0, clamp(v_WaterDepth / k_WaterMaxDepth, 0.0, 1.0));
    float alpha = albedo.a * pc.alphaScale * mix(1.0, depthFade, pc.isWater);

    o_Color = vec4(albedo.rgb * v_Tint * light, alpha);
}
