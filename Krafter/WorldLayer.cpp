#include <cstdlib>
#include <utility>

#include "imgui.h"

#include "Krafter/Core/Application.h"
#include "Krafter/Core/Event.h"
#include "Krafter/Core/Renderer.h"
#include "Krafter/Core/Window.h"
#include "Krafter/World/Biome.h"
#include "Krafter/World/Chunk.h"
#include "Krafter/WorldLayer.h"

namespace Krafter {

namespace {

glm::vec3 FindLandSpawn()
{
    constexpr int32_t k_MaxRadius = 2048;
    constexpr int32_t k_Step = 8;
    for (int32_t r = 0; r <= k_MaxRadius; r += k_Step) {
        for (int32_t dz = -r; dz <= r; dz += k_Step) {
            for (int32_t dx = -r; dx <= r; dx += k_Step) {
                if (r != 0 && std::abs(dx) != r && std::abs(dz) != r) {
                    continue;
                }
                const float x = static_cast<float>(dx);
                const float z = static_cast<float>(dz);
                if (Biome::At(x, z) == BiomeType::k_Ocean) {
                    continue;
                }
                const int32_t surface = Biome::SurfaceHeight(x, z);
                if (surface > Chunk::k_SeaLevel + 1) {
                    return glm::vec3(x + 0.5f, static_cast<float>(surface) + 3.0f, z + 0.5f);
                }
            }
        }
    }
    return glm::vec3(0.5f, 100.0f, 0.5f);
}

}

WorldLayer::WorldLayer(
    Window& window, Renderer& renderer, int32_t seed, GameMode mode,
    std::function<void()> onPause, std::function<void()> onToggleInventory)
    : Layer("World")
    , m_Window(window)
    , m_Renderer(renderer)
    , m_OnPause(std::move(onPause))
    , m_OnToggleInventory(std::move(onToggleInventory))
    , m_World(seed)
    , m_Player(window, m_World, FindLandSpawn(), glm::radians(80.0f), mode)
{
}

bool WorldLayer::NearBlock(Block block) const
{
    constexpr int k_Reach = 4;
    const glm::ivec3 center = glm::ivec3(glm::floor(m_Player.GetPosition()));
    for (int dx = -k_Reach; dx <= k_Reach; ++dx) {
        for (int dy = -k_Reach; dy <= k_Reach; ++dy) {
            for (int dz = -k_Reach; dz <= k_Reach; ++dz) {
                if (m_World.GetBlock(center + glm::ivec3(dx, dy, dz)) == block) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool WorldLayer::NearWorkbench() const
{
    return NearBlock(Block::k_Workbench);
}

bool WorldLayer::NearFurnace() const
{
    return NearBlock(Block::k_Furnace);
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
        const Block breaking = m_World.GetBlock(m_Player.GetBreakBlock());
        m_WorldRenderer.RenderBlockBreak(
            m_Player.GetBreakBlock(), m_Player.GetBreakProgress(), m_Player.GetViewProjection(),
            IsPlant(breaking) || IsGem(breaking));
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
