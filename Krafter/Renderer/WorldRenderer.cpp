#include <iostream>

#include "glad/gl.h"

#include "GLFW/glfw3.h"

#include "imgui.h"

#include "stb_image.h"

#include "Krafter/Renderer/WorldRenderer.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Sky.h"

namespace Krafter {

// Atlas tile the water lives in (column 6, bottom row) and how fast it animates.
static constexpr int32_t k_WaterTileX = 96;
static constexpr int32_t k_WaterTileY = 0;
static constexpr int32_t k_WaterTileSize = 16;
static constexpr double k_WaterFps = 12.0;

void WorldRenderer::BindChunkProgram(const glm::mat4& viewProjection, const Sky& sky)
{
    m_Texture->Bind(0);
    m_Program->Bind();
    m_Program->SetUniformMat4(0, viewProjection);
    m_Program->SetUniformInt(1, 0);
    m_Program->SetUniformVec3(2, sky.GetSunColor());
    m_Program->SetUniformVec3(3, sky.GetSunDirection());
    m_Program->SetUniformVec3(4, sky.GetAmbientColor());
}

void WorldRenderer::RenderChunkOpaque(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky)
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

void WorldRenderer::RenderChunkCross(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky)
{
    if (chunkMesh.GetCrossElementCount() == 0) {
        return;
    }
    BindChunkProgram(viewProjection, sky);
    m_Program->SetUniformFloat(5, 1.0f);
    m_Program->SetUniformFloat(6, 0.0f);
    chunkMesh.BindCross();
    glDrawElements(GL_TRIANGLES, chunkMesh.GetCrossElementCount(), GL_UNSIGNED_INT, nullptr);
}

void WorldRenderer::RenderChunkTransparent(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky)
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

void WorldRenderer::SetDepthMask(bool enabled)
{
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}

void WorldRenderer::SetCullFace(bool enabled)
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

void WorldRenderer::SetBlend(bool enabled)
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

void WorldRenderer::RenderBlockOutline(const glm::ivec3& blockPosition, const glm::mat4& viewProjection)
{
    m_OutlineProgram->Bind();
    m_OutlineProgram->SetUniformMat4(0, viewProjection);
    m_OutlineProgram->SetUniformVec3(1, glm::vec3(blockPosition));

    glLineWidth(1.0f);
    glBindVertexArray(m_OutlineVertexArray);
    glDrawElements(GL_LINES, m_OutlineElementCount, GL_UNSIGNED_INT, nullptr);
}

void WorldRenderer::RenderBlockBreak(const glm::ivec3& blockPosition, float progress, const glm::mat4& viewProjection)
{
    if (m_BreakFrameCount <= 0) {
        return;
    }

    // Pick the crack stage from progress and find its band in the strip. The
    // texture is loaded flipped (v = 1 at the top of the file, where the faint
    // first stage sits), so stage s climbs down from the top.
    int32_t stage = static_cast<int32_t>(progress * static_cast<float>(m_BreakFrameCount));
    stage = glm::clamp(stage, 0, m_BreakFrameCount - 1);
    const float span = 1.0f / static_cast<float>(m_BreakFrameCount);
    const float base = 1.0f - static_cast<float>(stage + 1) * span;

    m_BreakTexture->Bind(0);
    m_BreakProgram->Bind();
    m_BreakProgram->SetUniformMat4(0, viewProjection);
    m_BreakProgram->SetUniformVec3(1, glm::vec3(blockPosition));
    m_BreakProgram->SetUniformFloat(2, base);
    m_BreakProgram->SetUniformFloat(3, span);
    m_BreakProgram->SetUniformInt(4, 0);

    // Blend the cracks over the block's own face, pulled a hair toward the camera
    // so they sit on the surface without z-fighting, and without writing depth.
    // Culling is off so the front faces show whichever way the cube is wound; the
    // back faces fail the depth test against the already-drawn block anyway.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glDisable(GL_CULL_FACE);

    glBindVertexArray(m_BreakVertexArray);
    glDrawElements(GL_TRIANGLES, m_BreakElementCount, GL_UNSIGNED_INT, nullptr);

    glPolygonOffset(0.0f, 0.0f);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void WorldRenderer::AnimateWater()
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

void WorldRenderer::RenderImGui()
{
    ImGui::SliderFloat("Deep Water Opacity", &m_WaterOpacity, 0.0f, 1.0f);
}

WorldRenderer::WorldRenderer()
{
    BlockAtlas::LoadAtlases();

    m_Program = std::make_unique<ShaderProgram>("assets/shaders/default.vert.glsl", "assets/shaders/default.frag.glsl");
    m_Texture = std::make_unique<Texture2D>("assets/textures/blocks.png");

    // Load the water animation strip: a vertical stack of 16x16 frames. Alpha is
    // forced opaque so the shader alone controls water transparency.
    stbi_set_flip_vertically_on_load(false);
    int32_t waterWidth = 0;
    int32_t waterHeight = 0;
    int32_t waterChannels = 0;
    uint8_t* waterData = stbi_load("assets/textures/water_still.png", &waterWidth, &waterHeight, &waterChannels, 4);
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

    m_OutlineProgram = std::make_unique<ShaderProgram>("assets/shaders/outline.vert.glsl", "assets/shaders/outline.frag.glsl");

    // Crack overlay: a unit cube whose six faces each carry a full 0..1 UV, so
    // every face shows the whole crack tile. Five floats per vertex (xyz + uv).
    const float breakVertices[] = {
        // Front (z = 1)
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        // Back (z = 0)
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        // Left (x = 0)
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        // Right (x = 1)
        1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        // Bottom (y = 0)
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        // Top (y = 1)
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    };
    uint32_t breakIndices[36];
    for (uint32_t face = 0; face < 6; face++) {
        const uint32_t v = face * 4;
        const uint32_t i = face * 6;
        breakIndices[i + 0] = v + 0;
        breakIndices[i + 1] = v + 1;
        breakIndices[i + 2] = v + 2;
        breakIndices[i + 3] = v + 0;
        breakIndices[i + 4] = v + 2;
        breakIndices[i + 5] = v + 3;
    }
    m_BreakElementCount = sizeof(breakIndices) / sizeof(breakIndices[0]);

    glCreateVertexArrays(1, &m_BreakVertexArray);
    glCreateBuffers(1, &m_BreakVertexBuffer);
    glCreateBuffers(1, &m_BreakElementBuffer);

    glNamedBufferData(m_BreakVertexBuffer, sizeof(breakVertices), breakVertices, GL_STATIC_DRAW);
    glNamedBufferData(m_BreakElementBuffer, sizeof(breakIndices), breakIndices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(m_BreakVertexArray, 0, m_BreakVertexBuffer, 0, 5 * sizeof(float));
    glVertexArrayElementBuffer(m_BreakVertexArray, m_BreakElementBuffer);

    glEnableVertexArrayAttrib(m_BreakVertexArray, 0);
    glVertexArrayAttribBinding(m_BreakVertexArray, 0, 0);
    glVertexArrayAttribFormat(m_BreakVertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);

    glEnableVertexArrayAttrib(m_BreakVertexArray, 1);
    glVertexArrayAttribBinding(m_BreakVertexArray, 1, 0);
    glVertexArrayAttribFormat(m_BreakVertexArray, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));

    m_BreakProgram = std::make_unique<ShaderProgram>("assets/shaders/break.vert.glsl", "assets/shaders/break.frag.glsl");
    m_BreakTexture = std::make_unique<Texture2D>("assets/textures/destroy.png");

    // The strip stacks square frames, so the count is its height over its width.
    const glm::ivec2& breakSize = m_BreakTexture->GetSize();
    if (breakSize.x > 0) {
        m_BreakFrameCount = breakSize.y / breakSize.x;
    }
}

WorldRenderer::~WorldRenderer()
{
    glDeleteBuffers(1, &m_OutlineElementBuffer);
    glDeleteBuffers(1, &m_OutlineVertexBuffer);
    glDeleteVertexArrays(1, &m_OutlineVertexArray);

    glDeleteBuffers(1, &m_BreakElementBuffer);
    glDeleteBuffers(1, &m_BreakVertexBuffer);
    glDeleteVertexArrays(1, &m_BreakVertexArray);
}

} // namespace Krafter
