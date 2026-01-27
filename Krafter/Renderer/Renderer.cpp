#include <iostream>

#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "imgui.h"

#include "Krafter/Renderer/Renderer.h"

namespace Krafter {

Renderer::Renderer()
    : Layer("Renderer")
{
    assert(!s_instance);
    s_instance = this;

    PushLayer(m_camera = new Camera(glm::vec3(0.0f, 100.0f, 0.0f), glm::radians(80.0f)));
}

void Renderer::OnAttach()
{
    gladLoadGL(glfwGetProcAddress);

    m_versionName = glGetString(GL_VERSION);
    m_rendererName = glGetString(GL_RENDERER);

    glDebugMessageCallback(ApiDebugCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.470f, 0.655f, 1.0f, 1.0f);

    BlockAtlas::LoadAtlases();

    m_program = std::make_shared<ShaderProgram>("assets/default.vert.glsl", "assets/default.frag.glsl");
    m_texture = std::make_shared<Texture2D>("assets/texture.png");
}

void Renderer::OnRender()
{
    m_texture->Bind(0);
    m_program->Bind();
    m_program->SetUniformMat4(0, m_camera->GetViewProjection());
    m_program->SetUniformInt(1, 0);
    for (const auto& [position, chunkMesh] : m_chunkMeshes) {
        chunkMesh->Bind();
        glDrawElements(GL_TRIANGLES, chunkMesh->GetElementCount(), GL_UNSIGNED_INT, nullptr);
    }
}

void Renderer::OnRenderImGui()
{
    ImGui::Text("OpenGL Details:");
    ImGui::Text("Version: %s", m_versionName);
    ImGui::Text("Renderer: %s", m_rendererName);
}

void Renderer::GenerateChunkMesh(const ChunkMap& chunkMap, const glm::ivec2& chunkPosition)
{
    m_chunkMeshes[chunkPosition] = std::make_shared<ChunkMesh>(chunkMap, chunkPosition);
}

void Renderer::DeleteChunkMesh(const glm::ivec2& chunkPosition)
{
    m_chunkMeshes.erase(chunkPosition);
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
