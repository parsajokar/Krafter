#pragma once

#include <cstdint>
#include <functional>

#include "Krafter/Core/Layer.h"
#include "Krafter/Player.h"
#include "Krafter/Renderer/WorldRenderer.h"
#include "Krafter/World/Sky.h"
#include "Krafter/World/World.h"

namespace Krafter {

class Window;
class Renderer;
class Hotbar;

// The gameplay scene: owns the world and everything needed to play in it (the
// player, the sky, and the renderer that draws it).
class WorldLayer : public Layer {
public:
    // `onExitToMenu` is invoked when the player leaves the world (Escape), so the
    // application can tear this scene down and return to the main menu.
    WorldLayer(Window& window, Renderer& renderer, int32_t seed, std::function<void()> onExitToMenu);

    // The player's hotbar, exposed so the HUD overlay can share it without
    // depending on the player.
    Hotbar& GetHotbar()
    {
        return m_Player.GetHotbar();
    }

    // Hands control of the world to the player (mouse-look and movement). Called
    // by the main menu when "Play!" is pressed.
    void BeginPlay()
    {
        m_Player.SetControlled(true);
    }

private:
    void OnUpdate() override;
    void OnRender() override;
    void OnEvent(Event& event) override;
    void OnRenderImGui() override;

    Window& m_Window;
    Renderer& m_Renderer;

    std::function<void()> m_OnExitToMenu;

    Sky m_Sky;
    World m_World;
    WorldRenderer m_WorldRenderer;
    Player m_Player;
};

} // namespace Krafter
