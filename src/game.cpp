#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "window.h"
#include "renderer.h"
#include "game.h"

namespace Krafter
{

void Game::Init()
{
    _instance = new Game();
}

void Game::Deinit()
{
    delete _instance;
}

void Game::Run()
{
    float lastFrameTime = 0.0f;

    while (Window::Get()->IsOpen()) {
        Window::Get()->PollEvents();
        if (Window::Get()->IsKeyDown(Key::ESCAPE)) {
            Window::Get()->Close();
        }

        float currentFrameTime = Window::Get()->GetTime();
        _delta = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        Renderer::Get()->GetCamera().Update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings");
        ImGui::Text("FPS: %.2f", 1.0f / _delta);
        ImGui::Separator();
        Renderer::Get()->RenderImGui();
        ImGui::End();

        ImGui::Render();

        Renderer::Get()->ClearBuffers();
        Renderer::Get()->RenderChunkMeshes();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        Window::Get()->SwapBuffers();
    }
}

Game::Game()
    : _delta(0.0f)
{
    Window::Init();
    Renderer::Init();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "assets/editorconfig.ini";

    ImGui_ImplGlfw_InitForOpenGL(Window::Get()->GetId(), true);
    ImGui_ImplOpenGL3_Init("#version 450 core");

    _chunkMap.clear();
    for (uint32_t i = 0; i < 3; i++) {
        for (uint32_t j = 0; j < 3; j++) {
            _chunkMap.try_emplace(glm::ivec2(i, j), std::move(Chunk(glm::ivec2(i, j))));
        }
    }

    Renderer::Get()->GenerateAllChunkMeshes(_chunkMap);
}

Game::~Game()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    Renderer::Deinit();
    Window::Deinit();
}

} // namespace Krafter
