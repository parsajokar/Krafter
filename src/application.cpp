#include <filesystem>

#include "imgui.h"

#include "application.h"
#include "renderer/renderer.h"

namespace Krafter {

Application::Application(const ApplicationSpec& spec)
    : _spec(spec)
{
    assert(!_instance);
    _instance = this;

    if (!spec.workingDirectory.empty()) {
        std::filesystem::current_path(spec.workingDirectory);
    }

    _window = std::make_unique<Window>();

    PushLayer(_renderer = new Renderer());
    PushOverlay(_imguiOverlay = new ImGuiOverlay());
}

Application::~Application()
{
    for (auto it = _layerStack.rbegin(); it < _layerStack.rend(); it++) {
        (*it)->Detach();
        delete *it;
    }
}

void Application::PushLayer(Layer* layer)
{
    _layerStack.PushLayer(layer);
    layer->Attach();
}

void Application::PushOverlay(Layer* layer)
{
    _layerStack.PushOverlay(layer);
    layer->Attach();
}

void Application::Run()
{
    while (Window::Get()->IsOpen()) {
        _window->PollEvents();

        float currentFrameTime = _window->GetTime();
        _delta = currentFrameTime - _lastFrameTime;
        _lastFrameTime = currentFrameTime;

        for (Layer* layer : _layerStack) {
            layer->Update();
        }

        _renderer->ClearBuffers();

        for (Layer* layer : _layerStack) {
            layer->Render();
        }

        _imguiOverlay->Begin();
        ImGui::Text("FPS: %.2f", 1.0f / _delta);
        for (Layer* layer : _layerStack) {
            layer->RenderImGui();
        }
        _imguiOverlay->End();

        _window->SwapBuffers();
    }
}

} // namespace Krafter
