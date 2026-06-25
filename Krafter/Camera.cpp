#include "glm/gtc/matrix_transform.hpp"

#include "Krafter/Camera.h"

namespace Krafter {

Camera::Camera(const glm::vec3& position, float fov)
    : m_Position(position)
    , m_Yaw(0.0f)
    , m_Pitch(0.0f)
    , m_FieldOfView(fov)
{
    RecalculateViewProjection();
}

void Camera::SetPosition(const glm::vec3& position)
{
    m_Position = position;
    RecalculateViewProjection();
}

void Camera::SetRotation(float yaw, float pitch)
{
    m_Yaw = yaw;
    m_Pitch = pitch;
    RecalculateViewProjection();
}

glm::vec3 Camera::GetDirection() const
{
    return glm::normalize(glm::vec3(
        glm::cos(m_Yaw) * glm::cos(m_Pitch),
        glm::sin(m_Pitch),
        glm::sin(m_Yaw) * glm::cos(m_Pitch)));
}

void Camera::SetViewportSize(int32_t width, int32_t height)
{
    if (width > 0 && height > 0) {
        float aspectRatio = (float)width / (float)height;
        m_Projection = glm::perspective(m_FieldOfView, aspectRatio, 0.1f, 1000.0f);
        RecalculateViewProjection();
    }
}

void Camera::RecalculateViewProjection()
{
    const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(m_Position, m_Position + GetDirection(), up);
    m_ViewProjection = m_Projection * view;
}

} // namespace Krafter
