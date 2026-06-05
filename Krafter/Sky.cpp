#include <cmath>

#include "glm/gtc/constants.hpp"

#include "imgui.h"

#include "Krafter/Sky.h"
#include "Krafter/Window.h"

namespace Krafter {

void Sky::Update()
{
    s_TimeOfDay += Window::GetDelta() * s_Speed / k_BaseDayLengthSeconds;
    s_TimeOfDay -= std::floor(s_TimeOfDay);

    // The sun arcs east (+x) at dawn, overhead (+y) at noon, west (-x) at dusk;
    // the small +z tilt keeps z-facing faces softly lit.
    const float angle = s_TimeOfDay * glm::two_pi<float>();
    s_SunDirection = glm::normalize(glm::vec3(std::cos(angle), std::sin(angle), 0.25f));

    // Height of the sun above the horizon, -1 (midnight) .. 1 (noon).
    const float sunHeight = std::sin(angle);

    // Direct sunlight: gold near the horizon, white overhead, gone once set.
    const float directStrength = glm::clamp((sunHeight + 0.08f) / 0.18f, 0.0f, 1.0f);
    const float warmth = glm::clamp(sunHeight / 0.35f, 0.0f, 1.0f);
    const glm::vec3 goldenSun(1.0f, 0.50f, 0.20f);
    const glm::vec3 noonSun(1.0f, 0.95f, 0.85f);
    s_SunColor = glm::mix(goldenSun, noonSun, warmth) * directStrength * 0.6f;

    // Ambient skylight: neutral by day, cool blue at twilight, deep blue at night.
    const float dayMix = glm::clamp(sunHeight / 0.30f, 0.0f, 1.0f);
    const float nightMix = glm::clamp((-sunHeight - 0.10f) / 0.20f, 0.0f, 1.0f);
    const glm::vec3 dayAmbient(0.50f, 0.55f, 0.58f);
    const glm::vec3 twilightAmbient(0.18f, 0.24f, 0.40f);
    // Moonlit night: dim and cool, but never fully black.
    const glm::vec3 nightAmbient(0.16f, 0.19f, 0.28f);
    s_AmbientColor = glm::mix(glm::mix(twilightAmbient, dayAmbient, dayMix), nightAmbient, nightMix);

    // Sky colour: blue by day, orange at the horizon, near-black at night.
    const glm::vec3 daySky(0.470f, 0.655f, 1.0f);
    const glm::vec3 sunsetSky(0.80f, 0.45f, 0.32f);
    const glm::vec3 nightSky(0.05f, 0.07f, 0.16f);
    s_Color = glm::mix(glm::mix(sunsetSky, daySky, dayMix), nightSky, nightMix);
}

void Sky::RenderImGui()
{
    ImGui::Text("Sky:");
    ImGui::SliderFloat("Time of Day", &s_TimeOfDay, 0.0f, 1.0f);
    ImGui::SliderFloat("Day Speed", &s_Speed, 0.0f, 100.0f, "%.2fx");
    ImGui::Text("Sun Color: %.2f %.2f %.2f", s_SunColor.r, s_SunColor.g, s_SunColor.b);
    ImGui::Text("Ambient:   %.2f %.2f %.2f", s_AmbientColor.r, s_AmbientColor.g, s_AmbientColor.b);
}

} // namespace Krafter
