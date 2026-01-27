#pragma once

#include "glm/glm.hpp"

#include "Krafter/Layer.h"

namespace Krafter {

class Camera : public Layer {
public:
    Camera(const glm::vec3& position, float fov);
    ~Camera() = default;

    void OnAttach() override;
    void OnUpdate() override;
    void OnRenderImGui() override;

    void UpdateProjection();

    inline const glm::vec3& GetPosition() const
    {
        return m_position;
    }

    inline const glm::mat4& GetViewProjection() const
    {
        return m_viewProjection;
    }

private:
    void ToggleState();

    float m_speed;
    float m_sensitivity;

    bool m_isControlled;
    bool m_isSpaceReleased;

    glm::vec3 m_position;
    float m_fov;

    float m_pitch;
    float m_yaw;
    glm::vec2 m_lastCursorPosition;

    glm::mat4 m_projection;
    glm::mat4 m_viewProjection;
};

} // namespace Krafter
