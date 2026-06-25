#pragma once

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
    WorldLayer(Window& window, Renderer& renderer);

    // The player's hotbar, exposed so the HUD overlay can share it without
    // depending on the player.
    Hotbar& GetHotbar()
    {
        return m_Player.GetHotbar();
    }

private:
    void OnUpdate() override;
    void OnRender() override;
    void OnEvent(Event& event) override;
    void OnRenderImGui() override;

    Window& m_Window;
    Renderer& m_Renderer;

    Sky m_Sky;
    World m_World;
    WorldRenderer m_WorldRenderer;
    Player m_Player;
};

} // namespace Krafter
