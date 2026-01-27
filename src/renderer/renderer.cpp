#include <iostream>

#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "imgui.h"

#include "renderer/renderer.h"

namespace Krafter {

Renderer::Renderer()
    : Layer("Renderer")
{
    assert(!_instance);
    _instance = this;

    PushLayer(_camera = new Camera(glm::vec3(0.0f, 100.0f, 0.0f), glm::radians(80.0f)));
}

void Renderer::OnAttach()
{
    gladLoadGL(glfwGetProcAddress);

    _versionName = glGetString(GL_VERSION);
    _rendererName = glGetString(GL_RENDERER);

    glDebugMessageCallback(ApiDebugCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.470f, 0.655f, 1.0f, 1.0f);

    BlockAtlas::LoadAtlases();

    _program = std::make_shared<ShaderProgram>("assets/default.vert.glsl", "assets/default.frag.glsl");
    _texture = std::make_shared<Texture2D>("assets/texture.png");
}

void Renderer::OnRender()
{
    _texture->Bind(0);
    _program->Bind();
    _program->SetUniformMat4(0, _camera->GetViewProjection());
    _program->SetUniformInt(1, 0);
    for (const auto& [position, chunkMesh] : _chunkMeshes) {
        chunkMesh->Bind();
        glDrawElements(GL_TRIANGLES, chunkMesh->GetElementCount(), GL_UNSIGNED_INT, nullptr);
    }
}

void Renderer::OnRenderImGui()
{
    ImGui::Text("OpenGL Details:");
    ImGui::Text("Version: %s", _versionName);
    ImGui::Text("Renderer: %s", _rendererName);
}

void Renderer::GenerateChunkMesh(const ChunkMap& chunkMap, const glm::ivec2& chunkPosition)
{
    _chunkMeshes[chunkPosition] = std::make_shared<ChunkMesh>(chunkMap, chunkPosition);
}

void Renderer::DeleteChunkMesh(const glm::ivec2& chunkPosition)
{
    _chunkMeshes.erase(chunkPosition);
}

void Renderer::ClearBuffers()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::ApiDebugCallback(
    uint32_t source,
    uint32_t type,
    uint32_t id,
    uint32_t severity,
    int32_t length,
    const char* message,
    const void* userParam)
{
    std::cerr << "[OPENGL] " << message << std::endl;
    assert(severity != GL_DEBUG_SEVERITY_HIGH);
}

} // namespace Krafter
