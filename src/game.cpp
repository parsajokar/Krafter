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
    constexpr float FPS_DELAY = 0.5f;

    constexpr int32_t RENDER_DISTANCE = 12;
    constexpr float CHUNK_DELAY = 0.02f;

    uint32_t frameCount = 0;

    float lastFrameTime = 0.0f;
    float lastFpsCalculation = 0.0f;

    float lastChunkGeneration = 0.0f;
    float lastChunkDeletion = 0.0f;

    while (Window::Get()->IsOpen()) {
        Window::Get()->PollEvents();
        if (Window::Get()->IsKeyDown(Key::ESCAPE)) {
            Window::Get()->Close();
        }

        float currentFrameTime = Window::Get()->GetTime();
        _delta = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        if (Window::Get()->GetTime() > lastFpsCalculation + FPS_DELAY) {
            _fps = (float)frameCount / (Window::Get()->GetTime() - lastFpsCalculation);
            lastFpsCalculation = Window::Get()->GetTime();
            frameCount = 0;
        }

        Camera& cam = Renderer::Get()->GetCamera();
        cam.Update();

        glm::ivec2 chunkPosition = glm::ivec2(cam.GetPosition().x, cam.GetPosition().z) / Chunk::WIDTH;

        for (int32_t x = chunkPosition.x - RENDER_DISTANCE; x <= chunkPosition.x + RENDER_DISTANCE; x++) {
            for (int32_t z = chunkPosition.y - RENDER_DISTANCE; z <= chunkPosition.y + RENDER_DISTANCE; z++) {
                glm::ivec2 distance = glm::ivec2(x, z) - chunkPosition;
                if (distance.x * distance.x + distance.y * distance.y <= RENDER_DISTANCE * RENDER_DISTANCE) {
                    if (!_chunkMap.count(glm::ivec2(x, z)) && !_onChunkGenerationQueue.count(glm::ivec2(x, z))) {
                        _chunkGenerationQueue.push(glm::ivec2(x, z));
                        _onChunkGenerationQueue.insert(glm::ivec2(x, z));
                    }
                }
            }
        }

        for (auto& [k, e] : _chunkMap) {
            glm::ivec2 distance = chunkPosition - k;
            if (distance.x * distance.x + distance.y + distance.y > RENDER_DISTANCE * RENDER_DISTANCE && !_onChunkDeletionQueue.count(k)) {
                _chunkDeletionQueue.push(k);
                _onChunkDeletionQueue.insert(k);
            }
        }

        if (!_chunkGenerationQueue.empty() && Window::Get()->GetTime() > lastChunkGeneration + CHUNK_DELAY) {
            lastChunkGeneration = Window::Get()->GetTime();

            glm::ivec2& targetChunk = _chunkGenerationQueue.front();
            _chunkMap.emplace(targetChunk, std::move(Chunk(targetChunk)));
            Renderer::Get()->RegenerateChunkMesh(_chunkMap, targetChunk);

            _chunkGenerationQueue.pop();
            _onChunkGenerationQueue.erase(targetChunk);
        }

        if (!_chunkDeletionQueue.empty() && Window::Get()->GetTime() > lastChunkDeletion + CHUNK_DELAY) {
            lastChunkDeletion = Window::Get()->GetTime();

            glm::ivec2& targetChunk = _chunkDeletionQueue.front();
            _chunkMap.erase(targetChunk);
            Renderer::Get()->DeleteChunkMesh(targetChunk);

            _chunkDeletionQueue.pop();
            _onChunkDeletionQueue.erase(targetChunk);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings");
        ImGui::Text("FPS: %.2f", _fps);
        ImGui::Separator();
        Renderer::Get()->RenderImGui();
        ImGui::End();

        ImGui::Render();

        Renderer::Get()->ClearBuffers();
        Renderer::Get()->RenderChunkMeshes();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        Window::Get()->SwapBuffers();

        frameCount++;
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
