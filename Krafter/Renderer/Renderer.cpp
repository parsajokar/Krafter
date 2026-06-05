#include <iostream>

#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "imgui.h"

#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Sky.h"

namespace Krafter {

void Renderer::SetClearColor(const glm::vec3& color)
{
    m_ClearColor = color;
}

void Renderer::Clear()
{
    glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::RenderChunkMesh(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky)
{
    m_Texture->Bind(0);
    m_Program->Bind();
    m_Program->SetUniformMat4(0, viewProjection);
    m_Program->SetUniformInt(1, 0);
    m_Program->SetUniformVec3(2, sky.GetSunColor());
    m_Program->SetUniformVec3(3, sky.GetSunDirection());
    m_Program->SetUniformVec3(4, sky.GetAmbientColor());
    chunkMesh.Bind();
    glDrawElements(GL_TRIANGLES, chunkMesh.GetElementCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::RenderImGui()
{
    ImGui::Text("OpenGL Details:");
    ImGui::Text("Version: %s", m_VersionName);
    ImGui::Text("Renderer: %s", m_RendererName);
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

    BlockAtlas::LoadAtlases();

    m_Program = std::make_unique<ShaderProgram>("assets/default.vert.glsl", "assets/default.frag.glsl");
    m_Texture = std::make_unique<Texture2D>("assets/texture.png");
}

} // namespace Krafter
