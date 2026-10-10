#include <cstdlib>
#include <utility>

#include "imgui.h"

#include "Krafter/Engine/Event.h"
#include "Krafter/Engine/Graphics/Renderer.h"
#include "Krafter/Engine/Window.h"
#include "Krafter/Game/WorldLayer.h"
#include "Krafter/Rendering/RenderView.h"
#include "Krafter/World/Biome.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

namespace {

glm::vec3 FindLandSpawn(const Climate& climate)
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
                if (climate.BiomeAt(x, z) == BiomeType::k_Ocean) {
                    continue;
                }
                const int32_t surface = climate.SurfaceHeight(x, z);
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
    , m_World(seed, m_Jobs)
    , m_WorldRenderer(renderer)
    , m_ChunkRenderer(renderer, m_World, m_Jobs)
    , m_Player(window, m_World, FindLandSpawn(m_World.GetClimate()), glm::radians(80.0f), mode)
{
}

WorldLayer::~WorldLayer()
{
    m_Jobs.Shutdown();
}

void WorldLayer::OnUpdate()
{
    const int steps = m_Timestep.Advance(m_Window.GetDelta());
    for (int i = 0; i < steps; i++) {
        Tick(m_Timestep.GetStep());
    }
    m_Player.UpdateCamera(m_Timestep.GetAlpha());

    m_Renderer.SetClearColor(m_Sky.GetColor());
    m_WorldRenderer.AnimateWater();
    m_WorldRenderer.AnimateLava();

    m_World.Update(m_Player.GetPosition(), m_Window.GetDelta());
    m_ChunkRenderer.Update(m_Player.GetPosition(), m_Window.GetDelta());
}

void WorldLayer::Tick(float step)
{
    m_Sky.Update(step);
    m_Player.Tick(step);
    m_World.Tick(step, m_Player.GetPosition(), m_Player);
}

void WorldLayer::OnRender()
{
    const RenderView view(m_Player.GetCameraPosition(), m_Player.GetRelativeViewProjection());
    m_ChunkRenderer.PrepareFrame(view);

    m_ChunkRenderer.RenderPass(ChunkPass::k_Opaque, m_WorldRenderer, view, m_Sky);
    m_WorldRenderer.RenderDrops(m_World.GetDrops(), view, m_Timestep.GetAlpha());
    m_ChunkRenderer.RenderPass(ChunkPass::k_Cutout, m_WorldRenderer, view, m_Sky);
    m_ChunkRenderer.RenderPass(ChunkPass::k_Translucent, m_WorldRenderer, view, m_Sky);
    m_ChunkRenderer.RenderPass(ChunkPass::k_Transparent, m_WorldRenderer, view, m_Sky);

    if (m_Player.HasTarget()) {
        m_WorldRenderer.RenderBlockOutline(m_Player.GetTargetBlock(), view);
    }

    if (m_Player.IsBreaking()) {
        const Block breaking = m_World.GetBlock(m_Player.GetBreakBlock());
        m_WorldRenderer.RenderBlockBreak(
            m_Player.GetBreakBlock(), m_Player.GetBreakProgress(), view,
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

    m_Player.OnEvent(event);
}

void WorldLayer::OnRenderImGui()
{
    ImGui::Text("FPS: %.2f", m_Window.GetFps());

    const glm::vec3 position = m_Player.GetPosition();
    ImGui::Text("Current Biome: %s", Biome::Name(m_World.GetClimate().BiomeAt(position.x, position.z)));

    m_WorldRenderer.RenderImGui();
    m_Sky.RenderImGui();
    m_Player.RenderImGui();
    m_World.RenderImGui();
    m_ChunkRenderer.RenderImGui();
}

}
