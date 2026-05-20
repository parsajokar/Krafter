#include "imgui.h"

#include "Krafter/Game.h"
#include "Krafter/Window.h"

namespace Krafter {

GameLayer::GameLayer()
    : Layer("Game")
    , m_Camera(glm::vec3(0.0f, 100.0f, 0.0f), glm::radians(80.0f))
{
    Renderer::SetCamera(&m_Camera);
}

void GameLayer::OnUpdate()
{
    if (Window::IsKeyDown(Key::k_Escape)) {
        Window::Close();
    }

    m_Camera.Update();
    m_World.Update();
}

void GameLayer::OnRender()
{
    m_World.Render();
}

void GameLayer::OnRenderImGui()
{
    ImGui::Text("FPS: %.2f", 1.0f / Window::GetDelta());
    Renderer::RenderImGui();
    m_Camera.RenderImGui();
    m_World.RenderImGui();
}

GameApplication::GameApplication(const ApplicationSpecification& specification)
    : Application(specification)
{
    PushLayer(new GameLayer());
}

} // namespace Krafter
