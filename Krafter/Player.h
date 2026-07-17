#pragma once

#include "glm/glm.hpp"

#include "Krafter/Camera.h"
#include "Krafter/GameMode.h"
#include "Krafter/Hotbar.h"
#include "Krafter/Inventory.h"

namespace Krafter {

struct Event;
class Window;
class World;

class Player {
public:
    Player(Window& window, World& world, const glm::vec3& position, float fov, GameMode mode);

    void Update();
    void OnEvent(Event& event);
    void RenderImGui();

    void SetControlled(bool controlled);

    void SuspendForInventory();

    const glm::vec3& GetPosition() const
    {
        return m_Camera.GetPosition();
    }
    const glm::mat4& GetViewProjection() const
    {
        return m_Camera.GetViewProjection();
    }

    Hotbar& GetHotbar()
    {
        return m_Hotbar;
    }

    Inventory& GetInventory()
    {
        return m_Inventory;
    }

    bool HasTarget() const
    {
        return m_HasTarget;
    }
    const glm::ivec3& GetTargetBlock() const
    {
        return m_TargetBlock;
    }

    bool IsBreaking() const
    {
        return m_IsBreaking;
    }
    const glm::ivec3& GetBreakBlock() const
    {
        return m_BreakBlock;
    }
    float GetBreakProgress() const;

private:
    void ApplyControlMode();

    void UpdateSpectator();
    void UpdateSurvival();

    void BodyCellBounds(const glm::vec3& eye, glm::ivec3& outLo, glm::ivec3& outHi) const;

    bool CollidesAt(const glm::vec3& eye) const;

    bool OccupiesCell(const glm::ivec3& cell) const;

    bool RaycastTarget(glm::ivec3& hit, glm::ivec3& before) const;

    void UpdateBreaking();

    void CollectDrop(const Item& drop);

    void PlaceTargetBlock();

    static constexpr float k_DefaultFlySpeed = 50.0f;
    static constexpr float k_DefaultWalkSpeed = 5.6f;

    float m_Reach = 8.0f;
    float m_Width = 0.6f;
    float m_Height = 1.8f;
    float m_EyeHeight = 1.62f;
    float m_Gravity = 36.0f;
    float m_JumpSpeed = 9.0f;
    float m_TerminalSpeed = 60.0f;
    float m_PlaceInterval = 0.2f;

    Window& m_Window;
    World& m_World;
    Hotbar m_Hotbar;
    Inventory m_Inventory;
    Camera m_Camera;

    GameMode m_Mode;

    float m_Speed;
    float m_Sensitivity = 50.0f;

    bool m_IsControlled = false;

    bool m_PhysicsEnabled = false;

    float m_VerticalVelocity = 0.0f;
    bool m_OnGround = false;

    bool m_JumpHeld = false;

    bool m_PlaceHeld = false;
    float m_PlaceCooldown = 0.0f;

    glm::vec2 m_MoveInput = glm::vec2(0.0f);

    bool m_FirstMouse = true;
    glm::vec2 m_LastCursorPosition = glm::vec2(0.0f);

    bool m_HasTarget = false;
    glm::ivec3 m_TargetBlock = glm::ivec3(0);

    bool m_BreakHeld = false;
    bool m_IsBreaking = false;
    glm::ivec3 m_BreakBlock = glm::ivec3(0);
    float m_BreakProgress = 0.0f;
};

}
