#pragma once

#include <cstdint>

#include "glm/glm.hpp"

namespace Krafter {

class Camera {
public:
    Camera(const glm::vec3& position, float fov);

    void SetPosition(const glm::vec3& position);
    inline const glm::vec3& GetPosition() const
    {
        return m_Position;
    }

    void SetRotation(float yaw, float pitch);
    inline float GetYaw() const
    {
        return m_Yaw;
    }
    inline float GetPitch() const
    {
        return m_Pitch;
    }

    glm::vec3 GetDirection() const;

    void SetFieldOfView(float fov);
    inline float GetFieldOfView() const
    {
        return m_FieldOfView;
    }

    void SetViewportSize(int32_t width, int32_t height);

    inline const glm::mat4& GetViewProjection() const
    {
        return m_ViewProjection;
    }

private:
    void RecalculateProjection();
    void RecalculateViewProjection();

    glm::vec3 m_Position;
    float m_Yaw;
    float m_Pitch;
    float m_FieldOfView;

    float m_AspectRatio = 1.0f;

    glm::mat4 m_Projection = glm::mat4(1.0f);
    glm::mat4 m_ViewProjection = glm::mat4(1.0f);
};

}
