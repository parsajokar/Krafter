#pragma once

#include "glm/glm.hpp"

namespace Krafter {

class Camera {
public:
    Camera(const glm::vec3& position, float fov);

    void Update();
    void UpdateProjection();
    void RenderImGui();

    inline const glm::vec3& GetPosition() const
    {
        return m_Position;
    }

    inline const glm::mat4& GetViewProjection() const
    {
        return m_ViewProjection;
    }

private:
    void ToggleState();

    float m_Speed;
    float m_Sensitivity;

    bool m_IsControlled;
    bool m_IsSpaceReleased;

    glm::vec3 m_Position;
    float m_FieldOfView;

    float m_Pitch;
    float m_Yaw;
    glm::vec2 m_LastCursorPosition;

    glm::mat4 m_Projection;
    glm::mat4 m_ViewProjection;
};

} // namespace Krafter
