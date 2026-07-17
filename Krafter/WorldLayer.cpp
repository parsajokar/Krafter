#include <utility>

#include "imgui.h"

#include "Krafter/Core/Application.h"
#include "Krafter/Core/Event.h"
#include "Krafter/Core/Renderer.h"
#include "Krafter/Core/Window.h"
#include "Krafter/World/Biome.h"
#include "Krafter/WorldLayer.h"

namespace Krafter {

WorldLayer::WorldLayer(
    Window& window, Renderer& renderer, int32_t seed, GameMode mode,
    std::function<void()> onPause, std::function<void()> onToggleInventory)
    : Layer("World")
    , m_Window(window)
    , m_Renderer(renderer)
    , m_OnPause(std::move(onPause))
    , m_OnToggleInventory(std::move(onToggleInventory))
    , m_World(seed)
    , m_Player(window, m_World, glm::vec3(0.0f, 100.0f, 0.0f), glm::radians(80.0f), mode)
{
}

bool WorldLayer::NearWorkbench() const
{
    constexpr int k_Reach = 4;
    const glm::ivec3 center = glm::ivec3(glm::floor(m_Player.GetPosition()));
    for (int dx = -k_Reach; dx <= k_Reach; ++dx) {
        for (int dy = -k_Reach; dy <= k_Reach; ++dy) {
            for (int dz = -k_Reach; dz <= k_Reach; ++dz) {
                if (m_World.GetBlock(center + glm::ivec3(dx, dy, dz)) == Block::k_Workbench) {
                    return true;
                }
            }
        }
    }
    return false;
}

void WorldLayer::OnDetach()
{
    Application::Get().SetDebugUI(false);
}

void WorldLayer::OnUpdate()
{
    m_Sky.Update(m_Window.GetDelta());
    m_Renderer.SetClearColor(m_Sky.GetColor());
    m_WorldRenderer.AnimateWater();
    m_WorldRenderer.AnimateLava();

    m_Player.Update();
    m_World.Update(m_Player.GetPosition(), m_Window.GetDelta());
}

void WorldLayer::OnRender()
{
    m_World.Render(m_WorldRenderer, m_Player.GetViewProjection(), m_Sky);

    if (m_Player.HasTarget()) {
        m_WorldRenderer.RenderBlockOutline(m_Player.GetTargetBlock(), m_Player.GetViewProjection());
    }

    if (m_Player.IsBreaking()) {
        m_WorldRenderer.RenderBlockBreak(
            m_Player.GetBreakBlock(), m_Player.GetBreakProgress(), m_Player.GetViewProjection());
    }
}

void WorldLayer::OnEvent(Event& event)
{
    if (event.type == EventType::k_KeyPressed && event.key == Key::k_Escape && !event.isRepeat) {
        m_OnPause();
        event.handled = true;
        return;
    }

    if (event.type == EventType::k_KeyPressed && event.key == Key::k_E && !event.isRepeat) {
        m_OnToggleInventory();
        event.handled = true;
        return;
    }

    if (event.type == EventType::k_KeyPressed && event.key == Key::k_F3 && !event.isRepeat) {
        Application::Get().ToggleDebugUI();
        event.handled = true;
        return;
    }

    m_Player.OnEvent(event);
}

void WorldLayer::OnRenderImGui()
{
    ImGui::Text("FPS: %.2f", m_Window.GetFps());

    const glm::vec3 position = m_Player.GetPosition();
    ImGui::Text("Current Biome: %s", Biome::Name(Biome::At(position.x, position.z)));

    m_WorldRenderer.RenderImGui();
    m_Sky.RenderImGui();
    m_Player.RenderImGui();
    m_World.RenderImGui();
}

}
