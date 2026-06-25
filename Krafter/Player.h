#pragma once

#include "glm/glm.hpp"

#include "Krafter/Camera.h"
#include "Krafter/Hotbar.h"

namespace Krafter {

struct Event;
class Window;
class World;

// The controllable player: turns input into movement and look, drives its view
// camera, and handles the block interaction that aiming enables (targeting and
// breaking/placing against the world).
class Player {
public:
    Player(Window& window, World& world, const glm::vec3& position, float fov);

    void Update();
    void OnEvent(Event& event);
    void RenderImGui();

    const glm::vec3& GetPosition() const
    {
        return m_Camera.GetPosition();
    }
    const glm::mat4& GetViewProjection() const
    {
        return m_Camera.GetViewProjection();
    }

    // The player's hotbar (its quick-select block slots). The HUD reads and
    // mutates this through a reference, so it stays decoupled from the player.
    Hotbar& GetHotbar()
    {
        return m_Hotbar;
    }

    // The block the camera is currently looking at within reach, for the
    // highlight outline. Only valid while HasTarget() is true.
    bool HasTarget() const
    {
        return m_HasTarget;
    }
    const glm::ivec3& GetTargetBlock() const
    {
        return m_TargetBlock;
    }

private:
    // Toggles between controlling the player and freeing the cursor for the UI.
    void ToggleControl();
    void ApplyControlMode();

    // How far the player can reach to break, place, or target a block.
    static constexpr float k_Reach = 8.0f;

    Window& m_Window;
    World& m_World;
    Hotbar m_Hotbar;
    Camera m_Camera;

    float m_Speed = 50.0f;
    float m_Sensitivity = 50.0f;
    bool m_IsControlled = true;

    // Local move axes from held keys: x = strafe right, y = forward.
    glm::vec2 m_MoveInput = glm::vec2(0.0f);

    // Swallow the first mouse delta after entering control so the view doesn't jump.
    bool m_FirstMouse = true;
    glm::vec2 m_LastCursorPosition = glm::vec2(0.0f);

    bool m_HasTarget = false;
    glm::ivec3 m_TargetBlock = glm::ivec3(0);
};

} // namespace Krafter
