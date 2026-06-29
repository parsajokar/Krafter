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

// The controllable player: turns input into movement and look, drives its view
// camera, and handles the block interaction that aiming enables (targeting and
// breaking/placing against the world).
class Player {
public:
    Player(Window& window, World& world, const glm::vec3& position, float fov, GameMode mode);

    void Update();
    void OnEvent(Event& event);
    void RenderImGui();

    // Grabs or releases the player: capturing the cursor for mouse-look and
    // enabling input and physics, or freeing the cursor and freezing all three
    // for a menu. Used by the main menu to drop the player into the game once
    // "Play!" is pressed, and by the pause menu to freeze it.
    void SetControlled(bool controlled);

    // Releases look and movement input and frees the cursor for the inventory
    // screen, but leaves the physics running so momentum (such as a fall already
    // in progress) carries on. SetControlled(true) hands control back.
    void SuspendForInventory();

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

    // The player's main inventory (the grid above the hotbar). The inventory
    // screen reads it through a reference, the same way the HUD reads the hotbar.
    Inventory& GetInventory()
    {
        return m_Inventory;
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

    // The block currently being mined and how far the break has progressed
    // (0..1), for the crack overlay. Only valid while IsBreaking() is true.
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

    // Per-frame movement for each mode. Spectator flies free along the look
    // direction; survival walks the ground plane under gravity with collision.
    void UpdateSpectator();
    void UpdateSurvival();

    // The inclusive block-cell bounds the body spans with its eye at `eye`.
    void BodyCellBounds(const glm::vec3& eye, glm::ivec3& outLo, glm::ivec3& outHi) const;

    // True if the player's body, placed with its eye at `eye`, overlaps any solid
    // block. Drives the axis-separated collision resolution in survival mode.
    bool CollidesAt(const glm::vec3& eye) const;

    // True if `cell` lies within the player's body right now. Used to refuse a
    // block placement that would seal the survival player inside it.
    bool OccupiesCell(const glm::ivec3& cell) const;

    // Casts from the eye along the look direction to the first targetable block
    // within reach. Returns whether one was hit; `hit` is that block and `before`
    // the empty cell just in front of it (where a placement goes).
    bool RaycastTarget(glm::ivec3& hit, glm::ivec3& before) const;

    // Advances a held break: accumulates time against the aimed-at block's break
    // duration and removes it once that is reached, restarting whenever the aim
    // moves to a different block. A no-op unless the left button is held over a
    // block the selected tool can break.
    void UpdateBreaking();

    // Stows one dropped block, topping up an existing stack of the same block
    // that has room (up to Item::k_MaxStack) before starting a fresh stack in the
    // first empty slot, scanning the hotbar then the inventory grid each time. A
    // no-op for k_Air (nothing dropped) or when there is nowhere left to put it.
    void CollectDrop(Block drop);

    // Places the held hotbar block against the block under the crosshair, applying
    // the plant and self-trap rules. A no-op when nothing is targeted or the slot
    // is empty. Driven by the right mouse button, including while it is held.
    void PlaceTargetBlock();

    // How far the player can reach to break, place, or target a block.
    static constexpr float k_Reach = 8.0f;

    // Survival body and physics tuning, in blocks and blocks/second. The eye sits
    // k_EyeHeight above the feet; the body is a k_Width-wide, k_Height-tall box.
    static constexpr float k_Width = 0.6f;
    static constexpr float k_Height = 1.8f;
    static constexpr float k_EyeHeight = 1.62f;
    static constexpr float k_Gravity = 36.0f;
    static constexpr float k_JumpSpeed = 9.0f;
    static constexpr float k_TerminalSpeed = 60.0f;

    // Default movement speed (blocks/second) for each mode, used to seed the
    // adjustable m_Speed: brisk free-flight for spectator, a walk for survival.
    static constexpr float k_DefaultFlySpeed = 50.0f;
    static constexpr float k_DefaultWalkSpeed = 5.6f;

    // Seconds between placements while the right mouse button is held.
    static constexpr float k_PlaceInterval = 0.2f;

    Window& m_Window;
    World& m_World;
    Hotbar m_Hotbar;
    Inventory m_Inventory;
    Camera m_Camera;

    GameMode m_Mode;

    // Movement speed in blocks/second, adjustable from the debug UI. Seeded in the
    // constructor from the mode's default and used by both movement modes.
    float m_Speed;
    float m_Sensitivity = 50.0f;

    // Tracks whether the player is currently driving the camera. Starts false so
    // the world opens behind the main menu with the cursor free.
    bool m_IsControlled = false;

    // Whether the survival physics simulation integrates. Independent of control
    // so the inventory screen can release input yet keep gravity and momentum
    // going. Starts false (frozen at spawn behind the menu) and is enabled when
    // the player first takes control; a true menu pause turns it off again.
    bool m_PhysicsEnabled = false;

    // Survival vertical velocity and whether the body is resting on a block; only
    // used in survival mode (spectator flies and ignores both).
    float m_VerticalVelocity = 0.0f;
    bool m_OnGround = false;

    // Whether jump (space) is currently held, so holding it re-jumps on every
    // landing instead of needing a fresh press each time.
    bool m_JumpHeld = false;

    // Whether the right mouse button is held, and the countdown to the next
    // placement, so holding it keeps placing blocks on a fixed cadence.
    bool m_PlaceHeld = false;
    float m_PlaceCooldown = 0.0f;

    // Local move axes from held keys: x = strafe right, y = forward.
    glm::vec2 m_MoveInput = glm::vec2(0.0f);

    // Swallow the first mouse delta after entering control so the view doesn't jump.
    bool m_FirstMouse = true;
    glm::vec2 m_LastCursorPosition = glm::vec2(0.0f);

    bool m_HasTarget = false;
    glm::ivec3 m_TargetBlock = glm::ivec3(0);

    // Gradual mining state. While the left button is held over a breakable block,
    // m_BreakProgress accumulates toward that block's break time; aiming away or
    // releasing resets it. m_IsBreaking gates the crack overlay.
    bool m_BreakHeld = false;
    bool m_IsBreaking = false;
    glm::ivec3 m_BreakBlock = glm::ivec3(0);
    float m_BreakProgress = 0.0f;
};

} // namespace Krafter
