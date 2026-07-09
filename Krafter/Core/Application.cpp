#include <cassert>
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "Krafter/Core/Application.h"
#include "Krafter/Core/Event.h"
#include "Krafter/Core/Renderer.h"
#include "Krafter/Core/Window.h"

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
    m_Renderer = std::make_unique<Renderer>(*m_Window);

    InitImGui();
}

Application::~Application()
{
    // Nothing may still be executing on the GPU while layers free their Vulkan
    // resources (textures, buffers) below.
    m_Renderer->WaitIdle();

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

void Application::RemoveLayer(Layer* layer)
{
    layer->Detach();
    m_LayerStack.Remove(layer);
    delete layer;
}

void Application::QueueAfterFrame(std::function<void()> action)
{
    m_DeferredActions.push_back(std::move(action));
}

void Application::Run()
{
    while (m_Window->IsOpen()) {
        m_Window->PollEvents();

        for (Layer* layer : m_LayerStack) {
            layer->Update();
        }

        // Always run an ImGui frame so its input-capture state stays current, but
        // only build the debug overlay (and let layers contribute to it) when it
        // is toggled on. With no windows, ImGui captures nothing and draws nothing.
        // ImGui::Render only builds draw data on the CPU; it is recorded into the
        // command buffer after the render pass opens (Milestone 2).
        BeginImGui();
        if (m_ShowDebugUI) {
            ImGui::Begin("Settings");
            m_Renderer->RenderImGui();
            for (Layer* layer : m_LayerStack) {
                layer->RenderImGui();
            }
            ImGui::End();
        }
        EndImGui();

        // Acquire a swapchain image and open the render pass. A false return means
        // the swapchain was recreated (e.g. after a resize); skip drawing this frame.
        if (!m_Renderer->BeginFrame()) {
            continue;
        }

        for (Layer* layer : m_LayerStack) {
            layer->Render();
        }

        // The debug overlay draws on top of the scene, inside the same render pass.
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_Renderer->GetCommandBuffer());

        m_Renderer->EndFrame();

        // Drain deferred actions now that the layer stack is no longer being
        // iterated, so scene changes can safely add or remove layers.
        if (!m_DeferredActions.empty()) {
            std::vector<std::function<void()>> actions = std::move(m_DeferredActions);
            m_DeferredActions.clear();
            for (const std::function<void()>& action : actions) {
                action();
            }
        }
    }
}

void Application::OnEvent(Event& event)
{
    // F11 toggles fullscreen everywhere, ahead of the debug UI and every layer.
    if (event.type == EventType::k_KeyPressed && event.key == Key::k_F11 && !event.isRepeat) {
        m_Window->ToggleFullscreen();
        return;
    }

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

    ImGui_ImplGlfw_InitForVulkan(m_Window->GetId(), true);

    // The backend creates and owns its own descriptor pool when DescriptorPoolSize
    // is set, and uploads the font atlas lazily (RendererHasTextures), so no manual
    // pool or font upload is needed here.
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion = VK_API_VERSION_1_0;
    initInfo.Instance = m_Renderer->GetInstance();
    initInfo.PhysicalDevice = m_Renderer->GetPhysicalDevice();
    initInfo.Device = m_Renderer->GetDevice();
    initInfo.QueueFamily = m_Renderer->GetGraphicsQueueFamily();
    initInfo.Queue = m_Renderer->GetGraphicsQueue();
    initInfo.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE;
    initInfo.MinImageCount = Renderer::k_MaxFramesInFlight;
    initInfo.ImageCount = m_Renderer->GetSwapchainImageCount();
    initInfo.PipelineInfoMain.RenderPass = m_Renderer->GetRenderPass();
    initInfo.PipelineInfoMain.Subpass = 0;
    ImGui_ImplVulkan_Init(&initInfo);
}

void Application::ShutdownImGui()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Application::BeginImGui()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::EndImGui()
{
    // Builds the draw data on the CPU; it is recorded into the open render pass in
    // Run once BeginFrame has started the command buffer.
    ImGui::Render();
}

} // namespace Krafter
