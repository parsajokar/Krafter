#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"

#include "Krafter/Application.h"
#include "Krafter/Renderer/Camera.h"
#include "Krafter/Window.h"

namespace Krafter {

Camera::Camera(const glm::vec3& position, float fov)
    : Layer("Camera")
    , m_Speed(50.0f)
    , m_Sensitivity(50.0f)
    , m_IsControlled(true)
    , m_IsSpaceReleased(true)
    , m_Position(position)
    , m_FieldOfView(fov)
    , m_Pitch(0.0f)
    , m_Yaw(0.0f)
    , m_LastCursorPosition(Window::Get()->GetCursorPosition())
{
}

void Camera::OnAttach()
{
    UpdateProjection();
}

void Camera::OnUpdate()
{
    if (Window::Get()->IsKeyDown(Key::k_Space) && m_IsSpaceReleased) {
        ToggleState();
        m_IsSpaceReleased = false;
    }
    if (!Window::Get()->IsKeyDown(Key::k_Space)) {
        m_IsSpaceReleased = true;
    }

    if (m_IsControlled) {
        float delta = Application::Get()->GetDelta();

        glm::vec2 cursorPosition = Window::Get()->GetCursorPosition();
        glm::vec2 cursorOffset = cursorPosition - m_LastCursorPosition;
        m_LastCursorPosition = cursorPosition;

        m_Pitch -= cursorOffset.y * m_Sensitivity / 5000.0f;
        m_Yaw += cursorOffset.x * m_Sensitivity / 5000.0f;

        m_Pitch = glm::clamp(m_Pitch, glm::radians(-89.99f), glm::radians(89.99f));
        if (m_Yaw < 0.0f) {
            m_Yaw += glm::radians(360.0f);
        } else if (m_Yaw > glm::radians(360.0f)) {
            m_Yaw -= glm::radians(360.0f);
        }

        glm::vec3 direction = glm::normalize(glm::vec3(
            glm::cos(m_Yaw) * glm::cos(m_Pitch),
            glm::sin(m_Pitch),
            glm::sin(m_Yaw) * glm::cos(m_Pitch)));
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(direction, up));

        if (Window::Get()->IsKeyDown(Key::k_W)) {
            m_Position += direction * m_Speed * delta;
        }
        if (Window::Get()->IsKeyDown(Key::k_S)) {
            m_Position -= direction * m_Speed * delta;
        }
        if (Window::Get()->IsKeyDown(Key::k_D)) {
            m_Position += right * m_Speed * delta;
        }
        if (Window::Get()->IsKeyDown(Key::k_A)) {
            m_Position -= right * m_Speed * delta;
        }

        glm::mat4 transform = glm::lookAt(m_Position, m_Position + direction, up);
        m_ViewProjection = m_Projection * transform;
    }
}

void Camera::OnRenderImGui()
{
    ImGui::SliderFloat("Movement Speed", &m_Speed, 1.0f, 100.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &m_Sensitivity, 1.0f, 100.0f);
    ImGui::Text("Yaw: %.2f, Pitch: %.2f", glm::degrees(m_Yaw), glm::degrees(m_Pitch));
    ImGui::Text("Position: %.2f, %.2f, %.2f", m_Position.x, m_Position.y, m_Position.z);
}

void Camera::UpdateProjection()
{
    const glm::uvec2& size = Window::Get()->GetSize();
    if (size.x > 0 && size.y > 0) {
        float aspectRatio = (float)size.x / (float)size.y;
        m_Projection = glm::perspective(m_FieldOfView, aspectRatio, 0.1f, 1000.0f);
    }
}

void Camera::ToggleState()
{
    if (m_IsControlled) {
        Window::Get()->SetCursor(true);
    } else {
        Window::Get()->SetCursor(false);
        m_LastCursorPosition = Window::Get()->GetCursorPosition();
    }

    m_IsControlled = !m_IsControlled;
}

} // namespace Krafter
