#include "imgui.h"

#include "Krafter/Core/Event.h"
#include "Krafter/Core/Renderer.h"
#include "Krafter/Core/Window.h"
#include "Krafter/World/Biome.h"
#include "Krafter/WorldLayer.h"

namespace Krafter {

WorldLayer::WorldLayer(Window& window, Renderer& renderer)
    : Layer("World")
    , m_Window(window)
    , m_Renderer(renderer)
    , m_Player(window, m_World, glm::vec3(0.0f, 100.0f, 0.0f), glm::radians(80.0f))
{
}

void WorldLayer::OnUpdate()
{
    m_Sky.Update(m_Window.GetDelta());
    m_Renderer.SetClearColor(m_Sky.GetColor());
    m_WorldRenderer.AnimateWater();

    m_Player.Update();
    m_World.Update(m_Player.GetPosition());
}

void WorldLayer::OnRender()
{
    m_World.Render(m_WorldRenderer, m_Player.GetViewProjection(), m_Sky);

    if (m_Player.HasTarget()) {
        m_WorldRenderer.RenderBlockOutline(m_Player.GetTargetBlock(), m_Player.GetViewProjection());
    }
}

void WorldLayer::OnEvent(Event& event)
{
    if (event.type == EventType::k_KeyPressed && event.key == Key::k_Escape) {
        m_Window.Close();
        event.handled = true;
        return;
    }

    if (event.type == EventType::k_KeyPressed && event.key == Key::k_F11 && !event.isRepeat) {
        m_Window.ToggleFullscreen();
        event.handled = true;
        return;
    }

    m_Player.OnEvent(event);
}

void WorldLayer::OnRenderImGui()
{
    ImGui::Text("FPS: %.2f", 1.0f / m_Window.GetDelta());

    const glm::vec3 position = m_Player.GetPosition();
    ImGui::Text("Current Biome: %s", Biome::Name(Biome::At(position.x, position.z)));

    m_Renderer.RenderImGui();
    m_WorldRenderer.RenderImGui();
    m_Sky.RenderImGui();
    m_Player.RenderImGui();
    m_World.RenderImGui();
}

} // namespace Krafter
