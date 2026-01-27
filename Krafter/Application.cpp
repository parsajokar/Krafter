#include <filesystem>

#include "imgui.h"

#include "Krafter/Application.h"
#include "Krafter/Renderer/Renderer.h"

namespace Krafter {

Application::Application(const ApplicationSpec& spec)
    : m_spec(spec)
{
    assert(!s_instance);
    s_instance = this;

    if (!spec.workingDirectory.empty()) {
        std::filesystem::current_path(spec.workingDirectory);
    }

    m_window = std::make_unique<Window>();

    PushLayer(m_renderer = new Renderer());
    PushOverlay(m_imguiOverlay = new ImGuiOverlay());
}

Application::~Application()
{
    for (auto it = m_layerStack.rbegin(); it < m_layerStack.rend(); it++) {
        (*it)->Detach();
        delete *it;
    }
}

void Application::PushLayer(Layer* layer)
{
    m_layerStack.PushLayer(layer);
    layer->Attach();
}

void Application::PushOverlay(Layer* layer)
{
    m_layerStack.PushOverlay(layer);
    layer->Attach();
}

void Application::Run()
{
    while (Window::Get()->IsOpen()) {
        m_window->PollEvents();

        float currentFrameTime = m_window->GetTime();
        m_delta = currentFrameTime - m_lastFrameTime;
        m_lastFrameTime = currentFrameTime;

        for (Layer* layer : m_layerStack) {
            layer->Update();
        }

        m_renderer->ClearBuffers();

        for (Layer* layer : m_layerStack) {
            layer->Render();
        }

        m_imguiOverlay->Begin();
        ImGui::Text("FPS: %.2f", 1.0f / m_delta);
        for (Layer* layer : m_layerStack) {
            layer->RenderImGui();
        }
        m_imguiOverlay->End();

        m_window->SwapBuffers();
    }
}

} // namespace Krafter
