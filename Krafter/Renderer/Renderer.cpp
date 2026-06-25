#include <iostream>

#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "imgui.h"

#include "stb_image.h"

#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Sky.h"

namespace Krafter {

// Atlas tile the water lives in (column 6, bottom row) and how fast it animates.
static constexpr int32_t k_WaterTileX = 96;
static constexpr int32_t k_WaterTileY = 0;
static constexpr int32_t k_WaterTileSize = 16;
static constexpr double k_WaterFps = 12.0;

void Renderer::SetClearColor(const glm::vec3& color)
{
    m_ClearColor = color;
}

void Renderer::Clear()
{
    glClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::BindChunkProgram(const glm::mat4& viewProjection, const Sky& sky)
{
    m_Texture->Bind(0);
    m_Program->Bind();
    m_Program->SetUniformMat4(0, viewProjection);
    m_Program->SetUniformInt(1, 0);
    m_Program->SetUniformVec3(2, sky.GetSunColor());
    m_Program->SetUniformVec3(3, sky.GetSunDirection());
    m_Program->SetUniformVec3(4, sky.GetAmbientColor());
}

void Renderer::RenderChunkOpaque(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky)
{
    if (chunkMesh.GetOpaqueElementCount() == 0) {
        return;
    }
    BindChunkProgram(viewProjection, sky);
    m_Program->SetUniformFloat(5, 1.0f);
    m_Program->SetUniformFloat(6, 0.0f);
    chunkMesh.BindOpaque();
    glDrawElements(GL_TRIANGLES, chunkMesh.GetOpaqueElementCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::RenderChunkTransparent(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky)
{
    if (chunkMesh.GetTransparentElementCount() == 0) {
        return;
    }
    BindChunkProgram(viewProjection, sky);
    m_Program->SetUniformFloat(5, m_WaterOpacity);
    m_Program->SetUniformFloat(6, 1.0f);
    chunkMesh.BindTransparent();
    glDrawElements(GL_TRIANGLES, chunkMesh.GetTransparentElementCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::SetDepthMask(bool enabled)
{
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}

void Renderer::SetCullFace(bool enabled)
{
    // Chunk geometry winds counter-clockwise outward, so culling back faces hides
    // the interiors that would otherwise show through cutout foliage. Water is
    // drawn double-sided, so its pass turns culling back off.
    if (enabled) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void Renderer::SetBlend(bool enabled)
{
    // Other passes (the UI) toggle blending and leave it off, so the water pass
    // must turn it back on itself rather than trust the global state.
    if (enabled) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
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

void Renderer::AnimateWater()
{
    if (m_WaterFrameCount <= 0) {
        return;
    }

    const int32_t frame = static_cast<int32_t>(glfwGetTime() * k_WaterFps) % m_WaterFrameCount;
    if (frame == m_WaterFrame) {
        return;
    }
    m_WaterFrame = frame;

    const size_t offset = static_cast<size_t>(frame) * k_WaterTileSize * k_WaterTileSize * 4;
    m_Texture->UpdateRegion(k_WaterTileX, k_WaterTileY, k_WaterTileSize, k_WaterTileSize, m_WaterFrames.data() + offset);
}

void Renderer::RenderImGui()
{
    ImGui::Text("OpenGL Details:");
    ImGui::Text("Version: %s", m_VersionName);
    ImGui::Text("Renderer: %s", m_RendererName);
    ImGui::SliderFloat("Deep Water Opacity", &m_WaterOpacity, 0.0f, 1.0f);
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
    // LEQUAL so a decal drawn right after a coplanar face (the grass side fringe
    // over its dirt base) wins the equal-depth test and sits flush.
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    BlockAtlas::LoadAtlases();

    m_Program = std::make_unique<ShaderProgram>("assets/default.vert.glsl", "assets/default.frag.glsl");
    m_Texture = std::make_unique<Texture2D>("assets/texture.png");

    // Load the water animation strip: a vertical stack of 16x16 frames. Alpha is
    // forced opaque so the shader alone controls water transparency.
    stbi_set_flip_vertically_on_load(false);
    int32_t waterWidth = 0;
    int32_t waterHeight = 0;
    int32_t waterChannels = 0;
    uint8_t* waterData = stbi_load("assets/water_still.png", &waterWidth, &waterHeight, &waterChannels, 4);
    if (waterData && waterWidth == k_WaterTileSize && waterHeight >= k_WaterTileSize) {
        m_WaterFrameCount = waterHeight / k_WaterTileSize;
        const size_t bytes = static_cast<size_t>(m_WaterFrameCount) * k_WaterTileSize * k_WaterTileSize * 4;
        m_WaterFrames.assign(waterData, waterData + bytes);
        for (size_t i = 3; i < m_WaterFrames.size(); i += 4) {
            m_WaterFrames[i] = 255;
        }
    } else {
        std::cerr << "[FILE] Could not load animated water strip" << std::endl;
    }
    if (waterData) {
        stbi_image_free(waterData);
    }

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
