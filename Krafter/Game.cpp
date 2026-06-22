#include "imgui.h"

#include "Krafter/Event.h"
#include "Krafter/Game.h"
#include "Krafter/Renderer/Renderer.h"
#include "Krafter/UILayer.h"
#include "Krafter/Window.h"

namespace Krafter {

GameLayer::GameLayer(Window& window, Renderer& renderer)
    : Layer("Game")
    , m_Window(window)
    , m_Renderer(renderer)
    , m_Camera(window, glm::vec3(0.0f, 100.0f, 0.0f), glm::radians(80.0f))
{
}

void GameLayer::OnUpdate()
{
    m_Sky.Update(m_Window.GetDelta());
    m_Renderer.SetClearColor(m_Sky.GetColor());

    m_Camera.Update();
    m_World.Update(m_Camera.GetPosition());
}

void GameLayer::OnRender()
{
    m_World.Render(m_Renderer, m_Camera.GetViewProjection(), m_Sky);
}

void GameLayer::OnEvent(Event& event)
{
    if (event.type == EventType::k_KeyPressed && event.key == Key::k_Escape) {
        m_Window.Close();
        event.handled = true;
        return;
    }

    if (event.type == EventType::k_MouseButtonPressed && event.button == MouseButton::k_Left) {
        constexpr float k_Reach = 8.0f;
        glm::ivec3 hit;
        if (m_World.RaycastBlock(m_Camera.GetPosition(), m_Camera.GetDirection(), k_Reach, hit)) {
            m_World.SetBlock(hit, Block::k_Air);
        }
        event.handled = true;
        return;
    }

    m_Camera.OnEvent(event);
}

void GameLayer::OnRenderImGui()
{
    ImGui::Text("FPS: %.2f", 1.0f / m_Window.GetDelta());
    m_Renderer.RenderImGui();
    m_Sky.RenderImGui();
    m_Camera.RenderImGui();
    m_World.RenderImGui();
}

GameApplication::GameApplication(const ApplicationSpecification& specification)
    : Application(specification)
{
    PushLayer(new GameLayer(GetWindow(), GetRenderer()));
    PushOverlay(new UILayer(GetWindow()));
}

} // namespace Krafter
