#pragma once

#include "glm/glm.hpp"

namespace Krafter {

struct Event;
class Window;

class Camera {
public:
    Camera(Window& window, const glm::vec3& position, float fov);

    void Update();
    void UpdateProjection();
    void RenderImGui();
    void OnEvent(Event& event);

    inline const glm::vec3& GetPosition() const
    {
        return m_Position;
    }

    // Normalized forward (look) direction derived from yaw/pitch.
    glm::vec3 GetDirection() const;

    inline const glm::mat4& GetViewProjection() const
    {
        return m_ViewProjection;
    }

private:
    void ToggleState();
    void ApplyControlMode();

    Window& m_Window;

    float m_Speed;
    float m_Sensitivity;

    bool m_IsControlled;

    // Local axes: x = strafe right, y = forward.
    glm::vec2 m_MoveInput = glm::vec2(0.0f);

    // Swallow the first mouse delta after entering control so the view doesn't jump.
    bool m_FirstMouse = true;

    glm::vec3 m_Position;
    float m_FieldOfView;

    float m_Pitch;
    float m_Yaw;
    glm::vec2 m_LastCursorPosition;

    glm::mat4 m_Projection;
    glm::mat4 m_ViewProjection;
};

} // namespace Krafter
