#pragma once

#include "glm/glm.hpp"

namespace Krafter {

// Drives the day/night cycle: holds a normalized time-of-day in [0, 1) and
// derives the sun direction, sun colour, and ambient sky colour each frame.
class Sky {
public:
    static void Update();
    static void RenderImGui();

    inline static const glm::vec3& GetSunColor()
    {
        return s_SunColor;
    }
    inline static const glm::vec3& GetAmbientColor()
    {
        return s_AmbientColor;
    }
    inline static const glm::vec3& GetColor()
    {
        return s_Color;
    }
    inline static const glm::vec3& GetSunDirection()
    {
        return s_SunDirection;
    }

    // 0.0 = dawn, 0.25 = noon, 0.5 = dusk, 0.75 = midnight.
    inline static float GetTimeOfDay()
    {
        return s_TimeOfDay;
    }

private:
    // Real seconds for one full day at 1x speed; s_Speed scales the rate.
    static constexpr float k_BaseDayLengthSeconds = 600.0f;
    inline static float s_Speed = 1.0f;
    inline static float s_TimeOfDay = 0.2f;

    // Direct (directional) sunlight colour and the ambient sky-fill colour.
    inline static glm::vec3 s_SunColor = glm::vec3(1.0f);
    inline static glm::vec3 s_AmbientColor = glm::vec3(0.5f);

    inline static glm::vec3 s_Color = glm::vec3(0.470f, 0.655f, 1.0f);
    inline static glm::vec3 s_SunDirection = glm::vec3(0.0f, 1.0f, 0.0f);
};

} // namespace Krafter
