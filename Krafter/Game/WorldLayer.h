#pragma once

#include <cstdint>
#include <functional>

#include "Krafter/Engine/FixedTimestep.h"
#include "Krafter/Engine/JobSystem.h"
#include "Krafter/Engine/Layer.h"
#include "Krafter/Gameplay/Crafting.h"
#include "Krafter/Gameplay/GameMode.h"
#include "Krafter/Gameplay/Player.h"
#include "Krafter/Rendering/ChunkRenderer.h"
#include "Krafter/Rendering/WorldRenderer.h"
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
    ~WorldLayer() override;

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

    CraftingStations GetNearbyStations() const
    {
        return m_Player.GetNearbyStations();
    }

    void DropItem(const Item& item)
    {
        m_Player.DropItem(item);
    }

private:
    void OnUpdate() override;
    void Tick(float step);
    void OnRender() override;
    void OnEvent(Event& event) override;
    void OnRenderImGui() override;

    Window& m_Window;
    Renderer& m_Renderer;

    std::function<void()> m_OnPause;
    std::function<void()> m_OnToggleInventory;

    FixedTimestep m_Timestep { 1.0f / 60.0f, 5 };

    JobSystem m_Jobs;

    Sky m_Sky;
    World m_World;
    WorldRenderer m_WorldRenderer;
    ChunkRenderer m_ChunkRenderer;
    Player m_Player;
};

}
