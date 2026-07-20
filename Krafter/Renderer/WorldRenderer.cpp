#include <iostream>

#include "vulkan/vulkan.h"

#include "GLFW/glfw3.h"

#include "imgui.h"

#include "stb_image.h"

#include "Krafter/Core/Renderer.h"
#include "Krafter/Renderer/WorldRenderer.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Sky.h"

namespace Krafter {

namespace {

constexpr int32_t k_WaterTileX = 112;
constexpr int32_t k_WaterTileY = 0;
constexpr int32_t k_WaterTileSize = 16;

constexpr int32_t k_LavaTileX = 128;
constexpr int32_t k_LavaTileY = 0;

constexpr uint32_t k_VertexStride = 14 * sizeof(float);

std::vector<VkVertexInputAttributeDescription> ChunkAttributes()
{
    return {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, 3 * sizeof(float) },
        { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, 5 * sizeof(float) },
        { 3, 0, VK_FORMAT_R32_SFLOAT, 8 * sizeof(float) },
        { 4, 0, VK_FORMAT_R32_SFLOAT, 9 * sizeof(float) },
        { 5, 0, VK_FORMAT_R32G32B32_SFLOAT, 10 * sizeof(float) },
        { 6, 0, VK_FORMAT_R32_SFLOAT, 13 * sizeof(float) },
    };
}

int32_t LoadFluidStrip(const char* path, std::vector<uint8_t>& outFrames)
{
    stbi_set_flip_vertically_on_load(false);
    int32_t width = 0;
    int32_t height = 0;
    int32_t channels = 0;
    uint8_t* data = stbi_load(path, &width, &height, &channels, 4);
    int32_t frameCount = 0;
    if (data && (width == k_WaterTileSize || width == 2 * k_WaterTileSize) && height >= width) {
        const int32_t frameSize = width;
        const int32_t scale = frameSize / k_WaterTileSize;
        frameCount = height / frameSize;
        const size_t tilePixels = static_cast<size_t>(k_WaterTileSize) * k_WaterTileSize;
        outFrames.assign(static_cast<size_t>(frameCount) * tilePixels * 4, 0);
        for (int32_t f = 0; f < frameCount; f++) {
            for (int32_t ty = 0; ty < k_WaterTileSize; ty++) {
                for (int32_t tx = 0; tx < k_WaterTileSize; tx++) {
                    uint32_t r = 0, g = 0, b = 0;
                    for (int32_t sy = 0; sy < scale; sy++) {
                        for (int32_t sx = 0; sx < scale; sx++) {
                            const int32_t px = tx * scale + sx;
                            const int32_t py = f * frameSize + ty * scale + sy;
                            const uint8_t* p = data + (static_cast<size_t>(py) * width + px) * 4;
                            r += p[0];
                            g += p[1];
                            b += p[2];
                        }
                    }
                    const int32_t n = scale * scale;
                    uint8_t* o = outFrames.data()
                        + (static_cast<size_t>(f) * tilePixels + ty * k_WaterTileSize + tx) * 4;
                    o[0] = static_cast<uint8_t>(r / n);
                    o[1] = static_cast<uint8_t>(g / n);
                    o[2] = static_cast<uint8_t>(b / n);
                    o[3] = 255;
                }
            }
        }
    } else {
        std::cerr << "[FILE] Could not load animated strip " << path << std::endl;
    }
    if (data) {
        stbi_image_free(data);
    }
    return frameCount;
}

void DrawIndexed(VkCommandBuffer cmd, const GpuBuffer& vertexBuffer, const GpuBuffer& indexBuffer, uint32_t indexCount)
{
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);
}

}

WorldRenderer::ChunkPush WorldRenderer::MakeChunkPush(
    const glm::mat4& viewProjection, const Sky& sky, float alphaScale, float isWater) const
{
    ChunkPush push = {};
    push.viewProjection = viewProjection;
    push.sunColor = sky.GetSunColor();
    push.alphaScale = alphaScale;
    push.sunDirection = sky.GetSunDirection();
    push.isWater = isWater;
    push.ambientColor = sky.GetAmbientColor();
    return push;
}

void WorldRenderer::RenderChunkOpaque(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky)
{
    if (chunkMesh.GetOpaqueElementCount() == 0) {
        return;
    }
    VkCommandBuffer cmd = Renderer::Get().GetCommandBuffer();
    m_OpaquePipeline->Bind(cmd);
    m_OpaquePipeline->BindTextureSet(cmd, m_Texture->GetDescriptorSet());
    ChunkPush push = MakeChunkPush(viewProjection, sky, 1.0f, 0.0f);
    m_OpaquePipeline->PushConstants(cmd, &push, sizeof(push));
    chunkMesh.DrawOpaque(cmd);
}

void WorldRenderer::RenderChunkCross(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky)
{
    if (chunkMesh.GetCrossElementCount() == 0) {
        return;
    }
    VkCommandBuffer cmd = Renderer::Get().GetCommandBuffer();
    m_CrossPipeline->Bind(cmd);
    m_CrossPipeline->BindTextureSet(cmd, m_Texture->GetDescriptorSet());
    ChunkPush push = MakeChunkPush(viewProjection, sky, 1.0f, 0.0f);
    m_CrossPipeline->PushConstants(cmd, &push, sizeof(push));
    chunkMesh.DrawCross(cmd);
}

void WorldRenderer::RenderChunkTransparent(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky)
{
    if (chunkMesh.GetTransparentElementCount() == 0) {
        return;
    }
    VkCommandBuffer cmd = Renderer::Get().GetCommandBuffer();
    m_TransparentPipeline->Bind(cmd);
    m_TransparentPipeline->BindTextureSet(cmd, m_Texture->GetDescriptorSet());
    ChunkPush push = MakeChunkPush(viewProjection, sky, m_WaterOpacity, 1.0f);
    m_TransparentPipeline->PushConstants(cmd, &push, sizeof(push));
    chunkMesh.DrawTransparent(cmd);
}

void WorldRenderer::RenderChunkTranslucent(const ChunkMesh& chunkMesh, const glm::mat4& viewProjection, const Sky& sky)
{
    if (chunkMesh.GetTranslucentElementCount() == 0) {
        return;
    }
    VkCommandBuffer cmd = Renderer::Get().GetCommandBuffer();
    m_TranslucentPipeline->Bind(cmd);
    m_TranslucentPipeline->BindTextureSet(cmd, m_Texture->GetDescriptorSet());
    // isWater = 0 so opacity comes straight from the texture's alpha.
    ChunkPush push = MakeChunkPush(viewProjection, sky, 1.0f, 0.0f);
    m_TranslucentPipeline->PushConstants(cmd, &push, sizeof(push));
    chunkMesh.DrawTranslucent(cmd);
}

void WorldRenderer::AnimateTile(
    const std::vector<uint8_t>& frames, int32_t frameCount, int32_t& lastFrame,
    float fps, int32_t tileX, bool pingpong)
{
    if (frameCount <= 0) {
        return;
    }
    int32_t frame;
    if (pingpong) {
        const int32_t cycle = frameCount > 1 ? 2 * (frameCount - 1) : 1;
        const int32_t tick = static_cast<int32_t>(glfwGetTime() * fps) % cycle;
        frame = tick < frameCount ? tick : cycle - tick;
    } else {
        frame = static_cast<int32_t>(glfwGetTime() * fps) % frameCount;
    }
    if (frame == lastFrame) {
        return;
    }
    lastFrame = frame;

    const size_t offset = static_cast<size_t>(frame) * k_WaterTileSize * k_WaterTileSize * 4;
    m_Texture->UpdateRegion(tileX, k_WaterTileY, k_WaterTileSize, k_WaterTileSize, frames.data() + offset);
}

void WorldRenderer::AnimateWater()
{
    AnimateTile(m_WaterFrames, m_WaterFrameCount, m_WaterFrame, m_WaterFps, k_WaterTileX, false);
}

void WorldRenderer::AnimateLava()
{
    AnimateTile(m_LavaFrames, m_LavaFrameCount, m_LavaFrame, m_LavaFps, k_LavaTileX, true);
}

void WorldRenderer::RenderBlockOutline(const glm::ivec3& blockPosition, const glm::mat4& viewProjection)
{
    VkCommandBuffer cmd = Renderer::Get().GetCommandBuffer();
    m_OutlinePipeline->Bind(cmd);

    OutlinePush push = {};
    push.viewProjection = viewProjection;
    push.blockPosition = glm::vec3(blockPosition);
    m_OutlinePipeline->PushConstants(cmd, &push, sizeof(push));

    DrawIndexed(cmd, m_OutlineVertexBuffer, m_OutlineIndexBuffer, m_OutlineIndexCount);
}

void WorldRenderer::RenderBlockBreak(const glm::ivec3& blockPosition, float progress, const glm::mat4& viewProjection, bool cross)
{
    if (m_BreakFrameCount <= 0) {
        return;
    }

    int32_t stage = static_cast<int32_t>(progress * static_cast<float>(m_BreakFrameCount));
    stage = glm::clamp(stage, 0, m_BreakFrameCount - 1);
    const float span = 1.0f / static_cast<float>(m_BreakFrameCount);
    const float base = 1.0f - static_cast<float>(stage + 1) * span;

    VkCommandBuffer cmd = Renderer::Get().GetCommandBuffer();
    m_BreakPipeline->Bind(cmd);
    m_BreakPipeline->BindTextureSet(cmd, m_BreakTexture->GetDescriptorSet());

    BreakPush push = {};
    push.viewProjection = viewProjection;
    push.blockPosition = glm::vec3(blockPosition);
    push.frameBase = base;
    push.frameSpan = span;
    m_BreakPipeline->PushConstants(cmd, &push, sizeof(push));

    if (cross) {
        DrawIndexed(cmd, m_BreakCrossVertexBuffer, m_BreakCrossIndexBuffer, m_BreakCrossIndexCount);
    } else {
        DrawIndexed(cmd, m_BreakVertexBuffer, m_BreakIndexBuffer, m_BreakIndexCount);
    }
}

void WorldRenderer::RenderItemDrop(
    const glm::vec3& center, const glm::vec3& right, const glm::vec3& up,
    const glm::vec2& tileOrigin, bool fromItemAtlas, const glm::mat4& viewProjection)
{
    VkCommandBuffer cmd = Renderer::Get().GetCommandBuffer();
    m_ItemPipeline->Bind(cmd);
    m_ItemPipeline->BindTextureSet(
        cmd, (fromItemAtlas ? m_ItemTexture : m_Texture)->GetDescriptorSet());

    ItemPush push = {};
    push.viewProjection = viewProjection;
    push.center = center;
    push.right = right;
    push.up = up;
    push.tile = glm::vec3(tileOrigin, BlockAtlas::k_Step);
    m_ItemPipeline->PushConstants(cmd, &push, sizeof(push));

    DrawIndexed(cmd, m_ItemVertexBuffer, m_ItemIndexBuffer, m_ItemIndexCount);
}

void WorldRenderer::RenderImGui()
{
    ImGui::Text("World Renderer");
    ImGui::SliderFloat("Deep Water Opacity", &m_WaterOpacity, 0.0f, 1.0f);
    ImGui::SliderFloat("Water Anim FPS", &m_WaterFps, 0.0f, 30.0f);
    ImGui::SliderFloat("Lava Anim FPS", &m_LavaFps, 0.0f, 30.0f);
}

WorldRenderer::WorldRenderer()
{
    BlockAtlas::LoadAtlases();

    m_Texture = std::make_unique<Texture2D>("assets/textures/blocks.png");
    m_ItemTexture = std::make_unique<Texture2D>("assets/textures/items.png");

    m_WaterFrameCount = LoadFluidStrip("assets/textures/water.png", m_WaterFrames);
    m_LavaFrameCount = LoadFluidStrip("assets/textures/lava.png", m_LavaFrames);

    PipelineConfig config;
    config.vertPath = "assets/shaders/default.vert.spv";
    config.fragPath = "assets/shaders/default.frag.spv";
    config.vertexStride = k_VertexStride;
    config.attributes = ChunkAttributes();
    config.pushConstantSize = sizeof(ChunkPush);
    config.useTextureSet = true;
    config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config.depthTest = true;
    config.blend = true;

    config.cullMode = VK_CULL_MODE_BACK_BIT;
    config.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    config.depthWrite = true;
    m_OpaquePipeline = std::make_unique<Pipeline>(config);

    config.cullMode = VK_CULL_MODE_NONE;
    m_CrossPipeline = std::make_unique<Pipeline>(config);

    config.depthWrite = false;
    m_TransparentPipeline = std::make_unique<Pipeline>(config);

    // Translucent solids (ice): blended like water but a normal back-face-culled
    // cube, and shaded as opaque geometry (no water depth fade).
    config.cullMode = VK_CULL_MODE_BACK_BIT;
    m_TranslucentPipeline = std::make_unique<Pipeline>(config);

    Renderer& renderer = Renderer::Get();

    // clang-format off
    const float outlineCorners[] = {
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    };
    const uint32_t outlineEdges[] = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7,
    };
    // clang-format on
    m_OutlineIndexCount = static_cast<uint32_t>(std::size(outlineEdges));
    m_OutlineVertexBuffer = renderer.CreateDeviceLocalBuffer(
        outlineCorners, sizeof(outlineCorners), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_OutlineIndexBuffer = renderer.CreateDeviceLocalBuffer(
        outlineEdges, sizeof(outlineEdges), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    PipelineConfig outlineConfig;
    outlineConfig.vertPath = "assets/shaders/outline.vert.spv";
    outlineConfig.fragPath = "assets/shaders/outline.frag.spv";
    outlineConfig.vertexStride = 3 * sizeof(float);
    outlineConfig.attributes = { { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 } };
    outlineConfig.pushConstantSize = sizeof(OutlinePush);
    outlineConfig.useTextureSet = false;
    outlineConfig.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    outlineConfig.cullMode = VK_CULL_MODE_NONE;
    m_OutlinePipeline = std::make_unique<Pipeline>(outlineConfig);

    // clang-format off
    const float breakVertices[] = {
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    };
    // clang-format on
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
    m_BreakIndexCount = static_cast<uint32_t>(std::size(breakIndices));
    m_BreakVertexBuffer = renderer.CreateDeviceLocalBuffer(
        breakVertices, sizeof(breakVertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_BreakIndexBuffer = renderer.CreateDeviceLocalBuffer(
        breakIndices, sizeof(breakIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    // Two diagonal quads forming an X, matching the cross geometry of plants/gems
    // (see ChunkMesh::AddCrossToData) so their break overlay is cross-shaped too.
    // clang-format off
    const float breakCrossVertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    };
    // clang-format on
    uint32_t breakCrossIndices[12];
    for (uint32_t quad = 0; quad < 2; quad++) {
        const uint32_t v = quad * 4;
        const uint32_t i = quad * 6;
        breakCrossIndices[i + 0] = v + 0;
        breakCrossIndices[i + 1] = v + 1;
        breakCrossIndices[i + 2] = v + 2;
        breakCrossIndices[i + 3] = v + 0;
        breakCrossIndices[i + 4] = v + 2;
        breakCrossIndices[i + 5] = v + 3;
    }
    m_BreakCrossIndexCount = static_cast<uint32_t>(std::size(breakCrossIndices));
    m_BreakCrossVertexBuffer = renderer.CreateDeviceLocalBuffer(
        breakCrossVertices, sizeof(breakCrossVertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_BreakCrossIndexBuffer = renderer.CreateDeviceLocalBuffer(
        breakCrossIndices, sizeof(breakCrossIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    m_BreakTexture = std::make_unique<Texture2D>("assets/textures/destroy.png");
    const glm::ivec2& breakSize = m_BreakTexture->GetSize();
    if (breakSize.x > 0) {
        m_BreakFrameCount = breakSize.y / breakSize.x;
    }

    PipelineConfig breakConfig;
    breakConfig.vertPath = "assets/shaders/break.vert.spv";
    breakConfig.fragPath = "assets/shaders/break.frag.spv";
    breakConfig.vertexStride = 5 * sizeof(float);
    breakConfig.attributes = {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, 3 * sizeof(float) },
    };
    breakConfig.pushConstantSize = sizeof(BreakPush);
    breakConfig.useTextureSet = true;
    breakConfig.cullMode = VK_CULL_MODE_NONE;
    breakConfig.depthWrite = false;
    breakConfig.depthBias = true;
    breakConfig.depthBiasConstant = -1.0f;
    breakConfig.depthBiasSlope = -1.0f;
    m_BreakPipeline = std::make_unique<Pipeline>(breakConfig);

    // clang-format off
    const float itemVertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f,
        0.5f, -0.5f, 1.0f, 0.0f,
        0.5f, 0.5f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.0f, 1.0f,
    };
    // clang-format on
    const uint32_t itemIndices[] = { 0, 1, 2, 0, 2, 3 };
    m_ItemIndexCount = static_cast<uint32_t>(std::size(itemIndices));
    m_ItemVertexBuffer = renderer.CreateDeviceLocalBuffer(
        itemVertices, sizeof(itemVertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_ItemIndexBuffer = renderer.CreateDeviceLocalBuffer(
        itemIndices, sizeof(itemIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    PipelineConfig itemConfig;
    itemConfig.vertPath = "assets/shaders/item.vert.spv";
    itemConfig.fragPath = "assets/shaders/item.frag.spv";
    itemConfig.vertexStride = 4 * sizeof(float);
    itemConfig.attributes = {
        { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float) },
    };
    itemConfig.pushConstantSize = sizeof(ItemPush);
    itemConfig.useTextureSet = true;
    itemConfig.cullMode = VK_CULL_MODE_NONE;
    m_ItemPipeline = std::make_unique<Pipeline>(itemConfig);
}

WorldRenderer::~WorldRenderer()
{
    Renderer& renderer = Renderer::Get();
    renderer.DestroyBuffer(m_OutlineVertexBuffer);
    renderer.DestroyBuffer(m_OutlineIndexBuffer);
    renderer.DestroyBuffer(m_BreakVertexBuffer);
    renderer.DestroyBuffer(m_BreakIndexBuffer);
    renderer.DestroyBuffer(m_BreakCrossVertexBuffer);
    renderer.DestroyBuffer(m_BreakCrossIndexBuffer);
    renderer.DestroyBuffer(m_ItemVertexBuffer);
    renderer.DestroyBuffer(m_ItemIndexBuffer);
}

}
