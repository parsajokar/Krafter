#include <iostream>

#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "imgui.h"

#include "Krafter/Renderer/Renderer.h"

namespace Krafter {

Renderer::Renderer()
    : Layer("Renderer")
{
    assert(!s_Instance);
    s_Instance = this;

    PushLayer(m_Camera = new Camera(glm::vec3(0.0f, 100.0f, 0.0f), glm::radians(80.0f)));
}

void Renderer::OnAttach()
{
    gladLoadGL(glfwGetProcAddress);

    m_VersionName = glGetString(GL_VERSION);
    m_RendererName = glGetString(GL_RENDERER);

    glDebugMessageCallback(ApiDebugCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.470f, 0.655f, 1.0f, 1.0f);

    BlockAtlas::LoadAtlases();

    m_Program = std::make_shared<ShaderProgram>("assets/default.vert.glsl", "assets/default.frag.glsl");
    m_Texture = std::make_shared<Texture2D>("assets/texture.png");
}

void Renderer::OnRender()
{
    m_Texture->Bind(0);
    m_Program->Bind();
    m_Program->SetUniformMat4(0, m_Camera->GetViewProjection());
    m_Program->SetUniformInt(1, 0);
    for (const auto& [position, chunkMesh] : m_ChunkMeshes) {
        chunkMesh->Bind();
        glDrawElements(GL_TRIANGLES, chunkMesh->GetElementCount(), GL_UNSIGNED_INT, nullptr);
    }
}

void Renderer::OnRenderImGui()
{
    ImGui::Text("OpenGL Details:");
    ImGui::Text("Version: %s", m_VersionName);
    ImGui::Text("Renderer: %s", m_RendererName);
}

void Renderer::GenerateChunkMesh(const ChunkMap& chunkMap, const glm::ivec2& chunkPosition)
{
    m_ChunkMeshes[chunkPosition] = std::make_shared<ChunkMesh>(chunkMap, chunkPosition);
}

void Renderer::DeleteChunkMesh(const glm::ivec2& chunkPosition)
{
    m_ChunkMeshes.erase(chunkPosition);
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
