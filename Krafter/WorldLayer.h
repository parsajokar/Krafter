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

class WorldLayer : public Layer {
public:
    WorldLayer(
        Window& window, Renderer& renderer, int32_t seed, GameMode mode,
        std::function<void()> onPause, std::function<void()> onToggleInventory);

    Hotbar& GetHotbar()
    {
        return m_Player.GetHotbar();
    }

    Inventory& GetInventory()
    {
        return m_Player.GetInventory();
    }

    void BeginPlay()
    {
        m_Player.SetControlled(true);
    }

    void Pause()
    {
        m_Player.SetControlled(false);
    }
    void Resume()
    {
        m_Player.SetControlled(true);
    }

    void SuspendForInventory()
    {
        m_Player.SuspendForInventory();
    }

    bool NearWorkbench() const;

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

}
