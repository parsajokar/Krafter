#include <filesystem>

#include "imgui.h"

#include "Krafter/Application.h"
#include "Krafter/Renderer/Renderer.h"

namespace Krafter {

Application::Application(const ApplicationSpec& spec)
    : m_Spec(spec)
{
    assert(!s_Instance);
    s_Instance = this;

    if (!spec.workingDirectory.empty()) {
        std::filesystem::current_path(spec.workingDirectory);
    }

    m_Window = std::make_unique<Window>();

    PushLayer(m_Renderer = new Renderer());
    PushOverlay(m_ImGuiOverlay = new ImGuiOverlay());
}

Application::~Application()
{
    for (auto it = m_LayerStack.rbegin(); it < m_LayerStack.rend(); it++) {
        (*it)->Detach();
        delete *it;
    }
}

void Application::PushLayer(Layer* layer)
{
    m_LayerStack.PushLayer(layer);
    layer->Attach();
}

void Application::PushOverlay(Layer* layer)
{
    m_LayerStack.PushOverlay(layer);
    layer->Attach();
}

void Application::Run()
{
    while (Window::Get()->IsOpen()) {
        m_Window->PollEvents();

        float currentFrameTime = m_Window->GetTime();
        m_Delta = currentFrameTime - m_LastFrameTime;
        m_LastFrameTime = currentFrameTime;

        for (Layer* layer : m_LayerStack) {
            layer->Update();
        }

        m_Renderer->ClearBuffers();

        for (Layer* layer : m_LayerStack) {
            layer->Render();
        }

        m_ImGuiOverlay->Begin();
        ImGui::Text("FPS: %.2f", 1.0f / m_Delta);
        for (Layer* layer : m_LayerStack) {
            layer->RenderImGui();
        }
        m_ImGuiOverlay->End();

        m_Window->SwapBuffers();
    }
}

} // namespace Krafter
