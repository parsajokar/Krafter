#include <iostream>

#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "imgui.h"

#include "Krafter/Renderer/Renderer.h"

namespace Krafter {

void Renderer::Init()
{
    assert(!s_Renderer);
    s_Renderer = new Renderer();
}

void Renderer::Shutdown()
{
    delete s_Renderer;
}

void Renderer::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::RenderChunkMesh(const ChunkMesh& chunkMesh)
{
    s_Renderer->m_Texture->Bind(0);
    s_Renderer->m_Program->Bind();
    s_Renderer->m_Program->SetUniformMat4(0, s_Renderer->m_Camera->GetViewProjection());
    s_Renderer->m_Program->SetUniformInt(1, 0);
    chunkMesh.Bind();
    glDrawElements(GL_TRIANGLES, chunkMesh.GetElementCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::RenderImGui()
{
    ImGui::Text("OpenGL Details:");
    ImGui::Text("Version: %s", s_Renderer->m_VersionName);
    ImGui::Text("Renderer: %s", s_Renderer->m_RendererName);
}

void STDCALL Renderer::ApiDebugCallback(
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

Renderer::Renderer()
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

    m_Program = std::make_unique<ShaderProgram>("assets/default.vert.glsl", "assets/default.frag.glsl");
    m_Texture = std::make_unique<Texture2D>("assets/texture.png");
}

} // namespace Krafter
