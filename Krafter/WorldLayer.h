#pragma once

#include <cstdint>
#include <functional>

#include "Krafter/Core/Layer.h"
#include "Krafter/GameMode.h"
#include "Krafter/Player.h"
#include "Krafter/Renderer/WorldRenderer.h"
#include "Krafter/World/Sky.h"
#include "Krafter/World/World.h"

namespace Krafter {

class Window;
class Renderer;
class Hotbar;
class Inventory;

// The gameplay scene: owns the world and everything needed to play in it (the
// player, the sky, and the renderer that draws it).
class WorldLayer : public Layer {
public:
    // `mode` picks the control scheme (survival physics or spectator flight).
    // `onPause` is invoked when the player presses Escape, so the application can
    // bring up the pause menu over this scene; `onToggleInventory` when 'E' is
    // pressed, so it can bring up the inventory screen.
    WorldLayer(
        Window& window, Renderer& renderer, int32_t seed, GameMode mode,
        std::function<void()> onPause, std::function<void()> onToggleInventory);

    // The player's hotbar, exposed so the HUD overlay can share it without
    // depending on the player.
    Hotbar& GetHotbar()
    {
        return m_Player.GetHotbar();
    }

    // The player's inventory, exposed so the inventory overlay can share it
    // without depending on the player.
    Inventory& GetInventory()
    {
        return m_Player.GetInventory();
    }

    // Hands control of the world to the player (mouse-look and movement). Called
    // by the main menu when a play mode is chosen.
    void BeginPlay()
    {
        m_Player.SetControlled(true);
    }

    // Freezes the player and frees the cursor while the pause menu is up; Resume
    // hands control back. The owner pairs these with showing/hiding that menu.
    void Pause()
    {
        m_Player.SetControlled(false);
    }
    void Resume()
    {
        m_Player.SetControlled(true);
    }

    // Suspends player control for the inventory screen while leaving the physics
    // running, so momentum carries on. Resume() hands control back when it closes.
    void SuspendForInventory()
    {
        m_Player.SuspendForInventory();
    }

private:
    void OnDetach() override;
    void OnUpdate() override;
    void OnRender() override;
    void OnEvent(Event& event) override;
    void OnRenderImGui() override;

    Window& m_Window;
    Renderer& m_Renderer;

    std::function<void()> m_OnPause;
    std::function<void()> m_OnToggleInventory;

    Sky m_Sky;
    World m_World;
    WorldRenderer m_WorldRenderer;
    Player m_Player;
};

} // namespace Krafter
