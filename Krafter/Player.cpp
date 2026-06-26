#include "imgui.h"

#include "Krafter/Core/Event.h"
#include "Krafter/Core/Window.h"
#include "Krafter/Hotbar.h"
#include "Krafter/Player.h"
#include "Krafter/World/World.h"

namespace Krafter {

Player::Player(Window& window, World& world, const glm::vec3& position, float fov)
    : m_Window(window)
    , m_World(world)
    , m_Camera(position, fov)
{
    const glm::ivec2& size = m_Window.GetSize();
    m_Camera.SetViewportSize(size.x, size.y);
    ApplyControlMode();
}

void Player::Update()
{
    if (m_IsControlled) {
        float delta = m_Window.GetDelta();

        glm::vec3 direction = m_Camera.GetDirection();
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(direction, up));

        glm::vec3 position = m_Camera.GetPosition()
            + (right * m_MoveInput.x + direction * m_MoveInput.y) * m_Speed * delta;
        m_Camera.SetPosition(position);
    }

    glm::ivec3 hit;
    glm::ivec3 before;
    m_HasTarget = m_World.RaycastBlock(m_Camera.GetPosition(), m_Camera.GetDirection(), k_Reach, hit, before);
    if (m_HasTarget) {
        m_TargetBlock = hit;
    }
}

void Player::OnEvent(Event& event)
{
    if (event.type == EventType::k_MouseButtonPressed
        && (event.button == MouseButton::k_Left || event.button == MouseButton::k_Right)) {
        glm::ivec3 hit;
        glm::ivec3 before;
        if (m_World.RaycastBlock(m_Camera.GetPosition(), m_Camera.GetDirection(), k_Reach, hit, before)) {
            if (event.button == MouseButton::k_Left) {
                m_World.SetBlock(hit, Block::k_Air);
            } else {
                const Block block = m_Hotbar.GetSelectedBlock();
                if (block != Block::k_Air) {
                    // Replaceable foliage is overwritten in place; everything else
                    // is placed against the targeted face.
                    const glm::ivec3 target = IsPlant(m_World.GetBlock(hit)) ? hit : before;
                    m_World.PlaceBlock(target, block);
                }
            }
        }
        event.handled = true;
        return;
    }

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
            ToggleControl();
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

        float yaw = m_Camera.GetYaw() + offset.x * m_Sensitivity / 5000.0f;
        float pitch = m_Camera.GetPitch() - offset.y * m_Sensitivity / 5000.0f;

        pitch = glm::clamp(pitch, glm::radians(-89.99f), glm::radians(89.99f));
        if (yaw < 0.0f) {
            yaw += glm::radians(360.0f);
        } else if (yaw > glm::radians(360.0f)) {
            yaw -= glm::radians(360.0f);
        }
        m_Camera.SetRotation(yaw, pitch);
        break;
    }

    case EventType::k_WindowResized:
        m_Camera.SetViewportSize(event.size.x, event.size.y);
        break;

    default:
        break;
    }
}

void Player::RenderImGui()
{
    ImGui::SliderFloat("Movement Speed", &m_Speed, 1.0f, 100.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &m_Sensitivity, 1.0f, 100.0f);
    ImGui::Text("Yaw: %.2f, Pitch: %.2f", glm::degrees(m_Camera.GetYaw()), glm::degrees(m_Camera.GetPitch()));

    const glm::vec3 position = m_Camera.GetPosition();
    ImGui::Text("Position: %.2f, %.2f, %.2f", position.x, position.y, position.z);
}

void Player::ToggleControl()
{
    SetControlled(!m_IsControlled);
}

void Player::SetControlled(bool controlled)
{
    m_IsControlled = controlled;
    ApplyControlMode();

    // Reset movement and rebaseline the look so toggling doesn't drift or snap.
    m_MoveInput = glm::vec2(0.0f);
    m_FirstMouse = true;
}

void Player::ApplyControlMode()
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
