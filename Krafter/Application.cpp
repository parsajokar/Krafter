#include <filesystem>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Krafter/Application.h"
#include "Krafter/Renderer/Renderer.h"

namespace Krafter {

Application::Application(const ApplicationSpecification& specification)
    : m_Specification(specification)
{
    assert(!s_Application);
    s_Application = this;

    if (!m_Specification.workingDirectory.empty()) {
        std::filesystem::current_path(m_Specification.workingDirectory);
    }

    Window::Init();
    Renderer::Init();

    InitImGui();
}

Application::~Application()
{
    for (auto it = m_LayerStack.rbegin(); it < m_LayerStack.rend(); it++) {
        (*it)->Detach();
        delete *it;
    }

    ShutdownImGui();

    Renderer::Shutdown();
    Window::Shutdown();
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
    while (Window::IsOpen()) {
        Window::PollEvents();

        for (Layer* layer : m_LayerStack) {
            layer->Update();
        }

        Renderer::Clear();

        for (Layer* layer : m_LayerStack) {
            layer->Render();
        }

        BeginImGui();
        for (Layer* layer : m_LayerStack) {
            layer->RenderImGui();
        }
        EndImGui();

        Window::SwapBuffers();
    }
}

void Application::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "assets/editorconfig.ini";

    ImGui_ImplGlfw_InitForOpenGL(Window::GetId(), true);
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
}

void Application::EndImGui()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace Krafter
