#include <cassert>
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Krafter/Application.h"
#include "Krafter/Event.h"
#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Window.h"

namespace Krafter {

Application::Application(const ApplicationSpecification& specification)
    : m_Specification(specification)
{
    assert(!s_Application);
    s_Application = this;

    if (!m_Specification.workingDirectory.empty()) {
        std::filesystem::current_path(m_Specification.workingDirectory);
    }

    m_Window = std::make_unique<Window>();
    m_Window->SetEventCallback([this](Event& event) { OnEvent(event); });
    m_Renderer = std::make_unique<Renderer>();

    InitImGui();
}

Application::~Application()
{
    for (auto it = m_LayerStack.rbegin(); it < m_LayerStack.rend(); it++) {
        (*it)->Detach();
        delete *it;
    }

    ShutdownImGui();

    // m_Renderer is released before m_Window (reverse declaration order), so the
    // GL context is still alive while the renderer frees its resources.
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
    while (m_Window->IsOpen()) {
        m_Window->PollEvents();

        for (Layer* layer : m_LayerStack) {
            layer->Update();
        }

        m_Renderer->Clear();

        for (Layer* layer : m_LayerStack) {
            layer->Render();
        }

        BeginImGui();
        for (Layer* layer : m_LayerStack) {
            layer->RenderImGui();
        }
        EndImGui();

        m_Window->SwapBuffers();
    }
}

void Application::OnEvent(Event& event)
{
    // Let the debug UI consume input it is interacting with first.
    const ImGuiIO& io = ImGui::GetIO();
    if ((IsMouseEvent(event.type) && io.WantCaptureMouse)
        || (IsKeyEvent(event.type) && io.WantCaptureKeyboard)) {
        return;
    }

    // Reverse order, so overlays handle events before the layers beneath them.
    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend() && !event.handled; it++) {
        (*it)->HandleEvent(event);
    }
}

void Application::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "assets/editorconfig.ini";

    ImGui_ImplGlfw_InitForOpenGL(m_Window->GetId(), true);
    ImGui_ImplOpenGL3_Init("#version 450 core");
}

void Application::ShutdownImGui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Application::BeginImGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Settings");
}

void Application::EndImGui()
{
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace Krafter
