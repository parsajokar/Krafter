#pragma once

#include "glm/glm.hpp"

namespace Krafter {

class Sky {
public:
    void Update(float delta);
    void RenderImGui();

    inline const glm::vec3& GetSunColor() const
    {
        return m_SunColor;
    }
    inline const glm::vec3& GetAmbientColor() const
    {
        return m_AmbientColor;
    }
    inline const glm::vec3& GetColor() const
    {
        return m_Color;
    }
    inline const glm::vec3& GetSunDirection() const
    {
        return m_SunDirection;
    }

    inline float GetTimeOfDay() const
    {
        return m_TimeOfDay;
    }

private:
    static constexpr float k_BaseDayLengthSeconds = 600.0f;
    float m_Speed = 1.0f;
    float m_TimeOfDay = 0.2f;

    glm::vec3 m_SunColor = glm::vec3(1.0f);
    glm::vec3 m_AmbientColor = glm::vec3(0.5f);

    glm::vec3 m_Color = glm::vec3(0.470f, 0.655f, 1.0f);
    glm::vec3 m_SunDirection = glm::vec3(0.0f, 1.0f, 0.0f);
};

}
