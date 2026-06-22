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

void Renderer::RenderBlockOutline(const glm::ivec3& blockPosition, const glm::mat4& viewProjection)
{
    m_OutlineProgram->Bind();
    m_OutlineProgram->SetUniformMat4(0, viewProjection);
    m_OutlineProgram->SetUniformVec3(1, glm::vec3(blockPosition));

    glLineWidth(1.0f);
    glBindVertexArray(m_OutlineVertexArray);
    glDrawElements(GL_LINES, m_OutlineElementCount, GL_UNSIGNED_INT, nullptr);
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

    // Unit-cube wireframe for the targeted-block outline.
    const float cubeCorners[] = {
        0.0f, 0.0f, 0.0f, // 0
        1.0f, 0.0f, 0.0f, // 1
        1.0f, 1.0f, 0.0f, // 2
        0.0f, 1.0f, 0.0f, // 3
        0.0f, 0.0f, 1.0f, // 4
        1.0f, 0.0f, 1.0f, // 5
        1.0f, 1.0f, 1.0f, // 6
        0.0f, 1.0f, 1.0f, // 7
    };
    const uint32_t cubeEdges[] = {
        0, 1, 1, 2, 2, 3, 3, 0, // bottom
        4, 5, 5, 6, 6, 7, 7, 4, // top
        0, 4, 1, 5, 2, 6, 3, 7, // verticals
    };
    m_OutlineElementCount = sizeof(cubeEdges) / sizeof(cubeEdges[0]);

    glCreateVertexArrays(1, &m_OutlineVertexArray);
    glCreateBuffers(1, &m_OutlineVertexBuffer);
    glCreateBuffers(1, &m_OutlineElementBuffer);

    glNamedBufferData(m_OutlineVertexBuffer, sizeof(cubeCorners), cubeCorners, GL_STATIC_DRAW);
    glNamedBufferData(m_OutlineElementBuffer, sizeof(cubeEdges), cubeEdges, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(m_OutlineVertexArray, 0, m_OutlineVertexBuffer, 0, 3 * sizeof(float));
    glVertexArrayElementBuffer(m_OutlineVertexArray, m_OutlineElementBuffer);

    glEnableVertexArrayAttrib(m_OutlineVertexArray, 0);
    glVertexArrayAttribBinding(m_OutlineVertexArray, 0, 0);
    glVertexArrayAttribFormat(m_OutlineVertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);

    m_OutlineProgram = std::make_unique<ShaderProgram>("assets/outline.vert.glsl", "assets/outline.frag.glsl");
}

Renderer::~Renderer()
{
    glDeleteBuffers(1, &m_OutlineElementBuffer);
    glDeleteBuffers(1, &m_OutlineVertexBuffer);
    glDeleteVertexArrays(1, &m_OutlineVertexArray);
}

} // namespace Krafter
