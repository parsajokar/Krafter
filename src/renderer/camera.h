#pragma once

#include "glm/glm.hpp"

#include "layer.h"

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
        return _position;
    }

    inline const glm::mat4& GetViewProjection() const
    {
        return _viewProjection;
    }

private:
    void ToggleState();

    float _speed;
    float _sensitivity;

    bool _isControlled;
    bool _isSpaceReleased;

    glm::vec3 _position;
    float _fov;

    float _pitch;
    float _yaw;
    glm::vec2 _lastCursorPosition;

    glm::mat4 _projection;
    glm::mat4 _viewProjection;
};

} // namespace Krafter
