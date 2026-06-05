#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"

#include "Krafter/Camera.h"
#include "Krafter/Event.h"
#include "Krafter/Window.h"

namespace Krafter {

Camera::Camera(Window& window, const glm::vec3& position, float fov)
    : m_Window(window)
    , m_Speed(50.0f)
    , m_Sensitivity(50.0f)
    , m_IsControlled(true)
    , m_Position(position)
    , m_FieldOfView(fov)
    , m_Pitch(0.0f)
    , m_Yaw(0.0f)
    , m_LastCursorPosition(0.0f)
{
    UpdateProjection();
    ApplyControlMode();
}

void Camera::Update()
{
    if (!m_IsControlled) {
        return;
    }

    float delta = m_Window.GetDelta();

    glm::vec3 direction = glm::normalize(glm::vec3(
        glm::cos(m_Yaw) * glm::cos(m_Pitch),
        glm::sin(m_Pitch),
        glm::sin(m_Yaw) * glm::cos(m_Pitch)));
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(direction, up));

    m_Position += (right * m_MoveInput.x + direction * m_MoveInput.y) * m_Speed * delta;

    glm::mat4 transform = glm::lookAt(m_Position, m_Position + direction, up);
    m_ViewProjection = m_Projection * transform;
}

void Camera::UpdateProjection()
{
    const glm::ivec2& size = m_Window.GetSize();
    if (size.x > 0 && size.y > 0) {
        float aspectRatio = (float)size.x / (float)size.y;
        m_Projection = glm::perspective(m_FieldOfView, aspectRatio, 0.1f, 1000.0f);
    }
}

void Camera::RenderImGui()
{
    ImGui::SliderFloat("Movement Speed", &m_Speed, 1.0f, 100.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &m_Sensitivity, 1.0f, 100.0f);
    ImGui::Text("Yaw: %.2f, Pitch: %.2f", glm::degrees(m_Yaw), glm::degrees(m_Pitch));
    ImGui::Text("Position: %.2f, %.2f, %.2f", m_Position.x, m_Position.y, m_Position.z);
}

void Camera::OnEvent(Event& event)
{
    // Opposite keys cancel; auto-repeats are ignored so a held key counts once.
    auto applyMove = [&](float sign) {
        switch (event.key) {
        case Key::k_W:
            m_MoveInput.y += sign;
            break;
        case Key::k_S:
            m_MoveInput.y -= sign;
            break;
        case Key::k_D:
            m_MoveInput.x += sign;
            break;
        case Key::k_A:
            m_MoveInput.x -= sign;
            break;
        default:
            break;
        }
    };

    switch (event.type) {
    case EventType::k_KeyPressed:
        if (event.key == Key::k_Space && !event.isRepeat) {
            ToggleState();
            event.handled = true;
        } else if (!event.isRepeat) {
            applyMove(1.0f);
        }
        break;

    case EventType::k_KeyReleased:
        applyMove(-1.0f);
        break;

    case EventType::k_MouseMoved: {
        if (!m_IsControlled) {
            break;
        }
        if (m_FirstMouse) {
            m_LastCursorPosition = event.mouse;
            m_FirstMouse = false;
            break;
        }

        glm::vec2 offset = event.mouse - m_LastCursorPosition;
        m_LastCursorPosition = event.mouse;

        m_Pitch -= offset.y * m_Sensitivity / 5000.0f;
        m_Yaw += offset.x * m_Sensitivity / 5000.0f;

        m_Pitch = glm::clamp(m_Pitch, glm::radians(-89.99f), glm::radians(89.99f));
        if (m_Yaw < 0.0f) {
            m_Yaw += glm::radians(360.0f);
        } else if (m_Yaw > glm::radians(360.0f)) {
            m_Yaw -= glm::radians(360.0f);
        }
        break;
    }

    case EventType::k_WindowResized:
        UpdateProjection();
        break;

    default:
        break;
    }
}

void Camera::ToggleState()
{
    m_IsControlled = !m_IsControlled;
    ApplyControlMode();

    // Reset movement and rebaseline the look so toggling doesn't drift or snap.
    m_MoveInput = glm::vec2(0.0f);
    m_FirstMouse = true;
}

void Camera::ApplyControlMode()
{
    m_Window.SetCursor(!m_IsControlled);

    ImGuiIO& io = ImGui::GetIO();
    if (m_IsControlled) {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    } else {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }
}

} // namespace Krafter
