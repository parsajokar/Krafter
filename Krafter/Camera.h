#pragma once

#include <cstdint>

#include "glm/glm.hpp"

namespace Krafter {

// A passive view onto the world: a position and a yaw/pitch orientation plus a
// perspective projection, from which it derives the combined view-projection
// matrix. It does not read input or move itself; the Player aims and moves it.
class Camera {
public:
    Camera(const glm::vec3& position, float fov);

    void SetPosition(const glm::vec3& position);
    inline const glm::vec3& GetPosition() const
    {
        return m_Position;
    }

    // Orientation as yaw (around +y) and pitch (look up/down), in radians.
    void SetRotation(float yaw, float pitch);
    inline float GetYaw() const
    {
        return m_Yaw;
    }
    inline float GetPitch() const
    {
        return m_Pitch;
    }

    // Normalized forward (look) direction derived from yaw/pitch.
    glm::vec3 GetDirection() const;

    // Rebuilds the projection for a new viewport; degenerate sizes are ignored.
    void SetViewportSize(int32_t width, int32_t height);

    inline const glm::mat4& GetViewProjection() const
    {
        return m_ViewProjection;
    }

private:
    void RecalculateViewProjection();

    glm::vec3 m_Position;
    float m_Yaw;
    float m_Pitch;
    float m_FieldOfView;

    glm::mat4 m_Projection = glm::mat4(1.0f);
    glm::mat4 m_ViewProjection = glm::mat4(1.0f);
};

} // namespace Krafter
