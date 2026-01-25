#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"

#include "application.h"
#include "camera.h"
#include "window.h"

namespace Krafter {

Camera::Camera(const glm::vec3& position, float fov)
    : Layer("Camera")
    , _speed(50.0f)
    , _sensitivity(50.0f)
    , _isControlled(true)
    , _isSpaceReleased(true)
    , _position(position)
    , _fov(fov)
    , _pitch(0.0f)
    , _yaw(0.0f)
    , _lastCursorPosition(Window::Get()->GetCursorPosition())
{
}

void Camera::OnAttach()
{
    UpdateProjection();
}

void Camera::OnUpdate()
{
    if (Window::Get()->IsKeyDown(Key::SPACE) && _isSpaceReleased) {
        ToggleState();
        _isSpaceReleased = false;
    }
    if (!Window::Get()->IsKeyDown(Key::SPACE)) {
        _isSpaceReleased = true;
    }

    if (_isControlled) {
        float delta = Application::Get()->GetDelta();

        glm::vec2 cursorPosition = Window::Get()->GetCursorPosition();
        glm::vec2 cursorOffset = cursorPosition - _lastCursorPosition;
        _lastCursorPosition = cursorPosition;

        _pitch -= cursorOffset.y * _sensitivity / 5000.0f;
        _yaw += cursorOffset.x * _sensitivity / 5000.0f;

        _pitch = glm::clamp(_pitch, glm::radians(-89.99f), glm::radians(89.99f));
        if (_yaw < 0.0f) {
            _yaw += glm::radians(360.0f);
        } else if (_yaw > glm::radians(360.0f)) {
            _yaw -= glm::radians(360.0f);
        }

        glm::vec3 direction = glm::normalize(glm::vec3(
            glm::cos(_yaw) * glm::cos(_pitch),
            glm::sin(_pitch),
            glm::sin(_yaw) * glm::cos(_pitch)));
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(direction, up));

        if (Window::Get()->IsKeyDown(Key::W)) {
            _position += direction * _speed * delta;
        }
        if (Window::Get()->IsKeyDown(Key::S)) {
            _position -= direction * _speed * delta;
        }
        if (Window::Get()->IsKeyDown(Key::D)) {
            _position += right * _speed * delta;
        }
        if (Window::Get()->IsKeyDown(Key::A)) {
            _position -= right * _speed * delta;
        }

        glm::mat4 transform = glm::lookAt(_position, _position + direction, up);
        _viewProjection = _projection * transform;
    }
}

void Camera::OnRenderImGui()
{
    ImGui::SliderFloat("Movement Speed", &_speed, 1.0f, 100.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &_sensitivity, 1.0f, 100.0f);
    ImGui::Text("Yaw: %.2f, Pitch: %.2f", glm::degrees(_yaw), glm::degrees(_pitch));
    ImGui::Text("Position: %.2f, %.2f, %.2f", _position.x, _position.y, _position.z);
}

void Camera::UpdateProjection()
{
    const glm::uvec2& size = Window::Get()->GetSize();
    if (size.x > 0 && size.y > 0) {
        float aspectRatio = (float)size.x / (float)size.y;
        _projection = glm::perspective(_fov, aspectRatio, 0.1f, 1000.0f);
    }
}

void Camera::ToggleState()
{
    if (_isControlled) {
        Window::Get()->SetCursor(true);
    } else {
        Window::Get()->SetCursor(false);
        _lastCursorPosition = Window::Get()->GetCursorPosition();
    }

    _isControlled = !_isControlled;
}

} // namespace Krafter
