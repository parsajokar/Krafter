#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"

#include "Krafter/Application.h"
#include "Krafter/Renderer/Camera.h"
#include "Krafter/Window.h"

namespace Krafter {

Camera::Camera(const glm::vec3& position, float fov)
    : Layer("Camera")
    , m_speed(50.0f)
    , m_sensitivity(50.0f)
    , m_isControlled(true)
    , m_isSpaceReleased(true)
    , m_position(position)
    , m_fov(fov)
    , m_pitch(0.0f)
    , m_yaw(0.0f)
    , m_lastCursorPosition(Window::Get()->GetCursorPosition())
{
}

void Camera::OnAttach()
{
    UpdateProjection();
}

void Camera::OnUpdate()
{
    if (Window::Get()->IsKeyDown(Key::Space) && m_isSpaceReleased) {
        ToggleState();
        m_isSpaceReleased = false;
    }
    if (!Window::Get()->IsKeyDown(Key::Space)) {
        m_isSpaceReleased = true;
    }

    if (m_isControlled) {
        float delta = Application::Get()->GetDelta();

        glm::vec2 cursorPosition = Window::Get()->GetCursorPosition();
        glm::vec2 cursorOffset = cursorPosition - m_lastCursorPosition;
        m_lastCursorPosition = cursorPosition;

        m_pitch -= cursorOffset.y * m_sensitivity / 5000.0f;
        m_yaw += cursorOffset.x * m_sensitivity / 5000.0f;

        m_pitch = glm::clamp(m_pitch, glm::radians(-89.99f), glm::radians(89.99f));
        if (m_yaw < 0.0f) {
            m_yaw += glm::radians(360.0f);
        } else if (m_yaw > glm::radians(360.0f)) {
            m_yaw -= glm::radians(360.0f);
        }

        glm::vec3 direction = glm::normalize(glm::vec3(
            glm::cos(m_yaw) * glm::cos(m_pitch),
            glm::sin(m_pitch),
            glm::sin(m_yaw) * glm::cos(m_pitch)));
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(direction, up));

        if (Window::Get()->IsKeyDown(Key::W)) {
            m_position += direction * m_speed * delta;
        }
        if (Window::Get()->IsKeyDown(Key::S)) {
            m_position -= direction * m_speed * delta;
        }
        if (Window::Get()->IsKeyDown(Key::D)) {
            m_position += right * m_speed * delta;
        }
        if (Window::Get()->IsKeyDown(Key::A)) {
            m_position -= right * m_speed * delta;
        }

        glm::mat4 transform = glm::lookAt(m_position, m_position + direction, up);
        m_viewProjection = m_projection * transform;
    }
}

void Camera::OnRenderImGui()
{
    ImGui::SliderFloat("Movement Speed", &m_speed, 1.0f, 100.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &m_sensitivity, 1.0f, 100.0f);
    ImGui::Text("Yaw: %.2f, Pitch: %.2f", glm::degrees(m_yaw), glm::degrees(m_pitch));
    ImGui::Text("Position: %.2f, %.2f, %.2f", m_position.x, m_position.y, m_position.z);
}

void Camera::UpdateProjection()
{
    const glm::uvec2& size = Window::Get()->GetSize();
    if (size.x > 0 && size.y > 0) {
        float aspectRatio = (float)size.x / (float)size.y;
        m_projection = glm::perspective(m_fov, aspectRatio, 0.1f, 1000.0f);
    }
}

void Camera::ToggleState()
{
    if (m_isControlled) {
        Window::Get()->SetCursor(true);
    } else {
        Window::Get()->SetCursor(false);
        m_lastCursorPosition = Window::Get()->GetCursorPosition();
    }

    m_isControlled = !m_isControlled;
}

} // namespace Krafter
