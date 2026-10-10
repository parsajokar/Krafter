#pragma once

#include "glm/glm.hpp"

#include "Krafter/Gameplay/GameMode.h"
#include "Krafter/Gameplay/Hotbar.h"
#include "Krafter/Gameplay/Inventory.h"
#include "Krafter/Rendering/Camera.h"
#include "Krafter/World/DropSystem.h"

namespace Krafter {

struct Event;
struct CraftingStations;
class Window;
class World;

class Player : public ItemCollector {
public:
    Player(Window& window, World& world, const glm::vec3& position, float fov, GameMode mode);

    void Tick(float step);

    void UpdateCamera(float alpha);

    void OnEvent(Event& event);
    void RenderImGui();

    void SetControlled(bool controlled);

    void SuspendForInventory();

    const glm::vec3& GetPosition() const
    {
        return m_Position;
    }

    const glm::vec3& GetCameraPosition() const
    {
        return m_Camera.GetPosition();
    }
    const glm::mat4& GetRelativeViewProjection() const
    {
        return m_Camera.GetRelativeViewProjection();
    }

    Hotbar& GetHotbar()
    {
        return m_Hotbar;
    }

    Inventory& GetInventory()
    {
        return m_Inventory;
    }

    CraftingStations GetNearbyStations() const;

    bool HasRoomFor(const Item& item) override;
    Item Collect(const Item& item) override;

    void DropItem(const Item& item);

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

    void UpdateSpectator(float step);
    void UpdateSurvival(float step);

    void BodyCellBounds(const glm::vec3& eye, glm::ivec3& outLo, glm::ivec3& outHi) const;

    bool CollidesAt(const glm::vec3& eye) const;

    bool OccupiesCell(const glm::ivec3& cell) const;

    bool RaycastTarget(glm::ivec3& hit, glm::ivec3& before) const;

    void UpdateBreaking(float step);

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

    glm::vec3 m_Position;
    glm::vec3 m_PreviousPosition;

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

    int m_GiveQuantity = 1;
};

}
