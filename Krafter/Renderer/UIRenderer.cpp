#include <algorithm>

#include "vulkan/vulkan.h"

#include "glm/gtc/matrix_transform.hpp"

#include "Krafter/Core/Renderer.h"
#include "Krafter/Core/Window.h"
#include "Krafter/Renderer/UIRenderer.h"

namespace Krafter {

UIRenderer::UIRenderer(Window& window)
    : m_Window(window)
{
    Renderer& renderer = Renderer::Get();

    // clang-format off
    const float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
    };
    // clang-format on
    const uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };
    m_QuadVertexBuffer = renderer.CreateDeviceLocalBuffer(vertices, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_QuadIndexBuffer = renderer.CreateDeviceLocalBuffer(indices, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    const uint8_t white[4] = { 255, 255, 255, 255 };
    m_WhiteTexture = std::make_unique<Texture2D>(white, 1, 1);

    for (DynamicBuffer& dynamic : m_Dynamic) {
        dynamic.buffer = renderer.CreateHostVisibleBuffer(
            k_MaxDynamicVertices * 4 * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &dynamic.mapped);
    }

    PipelineConfig config;
    config.vertPath = "assets/shaders/ui.vert.spv";
    config.fragPath = "assets/shaders/ui.frag.spv";
    config.vertexStride = 4 * sizeof(float);
    config.attributes = {
        { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float) },
    };
    config.pushConstantSize = sizeof(UIPush);
    config.useTextureSet = true;
    config.cullMode = VK_CULL_MODE_NONE;
    config.depthTest = false;
    config.depthWrite = false;
    config.blend = true;
    m_Pipeline = std::make_unique<Pipeline>(config);

    config.srcColorFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    config.dstColorFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    m_InvertPipeline = std::make_unique<Pipeline>(config);
}

UIRenderer::~UIRenderer()
{
    Renderer& renderer = Renderer::Get();
    renderer.DestroyBuffer(m_QuadVertexBuffer);
    renderer.DestroyBuffer(m_QuadIndexBuffer);
    for (DynamicBuffer& dynamic : m_Dynamic) {
        renderer.DestroyBuffer(dynamic.buffer);
    }
}

void UIRenderer::Begin()
{
    const glm::ivec2& size = m_Window.GetSize();
    m_Projection = glm::ortho(0.0f, static_cast<float>(size.x), static_cast<float>(size.y), 0.0f);
}

void UIRenderer::End()
{
    ClearScissor();
}

void UIRenderer::SetScissor(const glm::vec2& position, const glm::vec2& size)
{
    const VkExtent2D extent = Renderer::Get().GetSwapchainExtent();
    const int32_t x = std::clamp(static_cast<int32_t>(position.x), 0, static_cast<int32_t>(extent.width));
    const int32_t y = std::clamp(static_cast<int32_t>(position.y), 0, static_cast<int32_t>(extent.height));
    const uint32_t w = std::min(static_cast<uint32_t>(std::max(size.x, 0.0f)), extent.width - x);
    const uint32_t h = std::min(static_cast<uint32_t>(std::max(size.y, 0.0f)), extent.height - y);

    VkRect2D scissor = {};
    scissor.offset = { x, y };
    scissor.extent = { w, h };
    vkCmdSetScissor(Renderer::Get().GetCommandBuffer(), 0, 1, &scissor);
}

void UIRenderer::ClearScissor()
{
    VkRect2D scissor = {};
    scissor.extent = Renderer::Get().GetSwapchainExtent();
    vkCmdSetScissor(Renderer::Get().GetCommandBuffer(), 0, 1, &scissor);
}

void UIRenderer::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
{
    DrawQuad(position, size, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), color, nullptr);
}

void UIRenderer::DrawQuad(
    const glm::vec2& position, const glm::vec2& size, const Texture2D& texture,
    const glm::vec4& uvRect, const glm::vec4& tint)
{
    DrawQuad(position, size, uvRect, tint, &texture);
}

void UIRenderer::DrawQuad(
    const glm::vec2& position, const glm::vec2& size,
    const glm::vec4& uvRect, const glm::vec4& tint, const Texture2D* texture)
{
    VkCommandBuffer cmd = Renderer::Get().GetCommandBuffer();
    m_Pipeline->Bind(cmd);
    m_Pipeline->BindTextureSet(cmd, (texture ? *texture : *m_WhiteTexture).GetDescriptorSet());

    UIPush push = {};
    push.projection = m_Projection;
    push.transform = glm::vec4(position, size);
    push.uvRect = uvRect;
    push.tint = tint;
    push.useTexture = texture ? 1 : 0;
    push.invert = 0;
    m_Pipeline->PushConstants(cmd, &push, sizeof(push));

    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_QuadVertexBuffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_QuadIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
}

void UIRenderer::DrawQuadInverted(
    const glm::vec2& position, const glm::vec2& size, const Texture2D& texture, const glm::vec4& uvRect)
{
    VkCommandBuffer cmd = Renderer::Get().GetCommandBuffer();
    m_InvertPipeline->Bind(cmd);
    m_InvertPipeline->BindTextureSet(cmd, texture.GetDescriptorSet());

    UIPush push = {};
    push.projection = m_Projection;
    push.transform = glm::vec4(position, size);
    push.uvRect = uvRect;
    push.tint = glm::vec4(1.0f);
    push.useTexture = 1;
    push.invert = 1;
    m_InvertPipeline->PushConstants(cmd, &push, sizeof(push));

    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_QuadVertexBuffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_QuadIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
}

void UIRenderer::DrawQuad(
    const std::array<glm::vec2, 4>& corners, const std::array<glm::vec2, 4>& uvs,
    const Texture2D& texture, const glm::vec4& tint)
{
    Renderer& renderer = Renderer::Get();

    const uint32_t frame = renderer.GetCurrentFrameIndex();
    if (frame != m_DynamicFrame) {
        m_DynamicFrame = frame;
        m_DynamicVertexOffset = 0;
    }
    if (m_DynamicVertexOffset + 4 > k_MaxDynamicVertices) {
        return;
    }

    const uint32_t base = m_DynamicVertexOffset;
    float* vertices = static_cast<float*>(m_Dynamic[frame].mapped) + base * 4;
    for (int i = 0; i < 4; i++) {
        vertices[i * 4 + 0] = corners[i].x;
        vertices[i * 4 + 1] = corners[i].y;
        vertices[i * 4 + 2] = uvs[i].x;
        vertices[i * 4 + 3] = uvs[i].y;
    }
    const VkDeviceSize writeOffset = static_cast<VkDeviceSize>(base) * 4 * sizeof(float);
    renderer.FlushHostBuffer(m_Dynamic[frame].buffer, writeOffset, 4 * 4 * sizeof(float));
    m_DynamicVertexOffset += 4;

    VkCommandBuffer cmd = renderer.GetCommandBuffer();
    m_Pipeline->Bind(cmd);
    m_Pipeline->BindTextureSet(cmd, texture.GetDescriptorSet());

    UIPush push = {};
    push.projection = m_Projection;
    push.transform = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    push.uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    push.tint = tint;
    push.useTexture = 1;
    push.invert = 0;
    m_Pipeline->PushConstants(cmd, &push, sizeof(push));

    const VkDeviceSize offset = base * 4 * sizeof(float);
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_Dynamic[frame].buffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_QuadIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
}

glm::vec4 UIRenderer::SpriteUVRect(
    const glm::vec2& spritePos, const glm::vec2& spriteSize, const glm::vec2& texSize)
{
    const glm::vec2 uvMin = spritePos / texSize;
    const glm::vec2 uvMax = (spritePos + spriteSize) / texSize;
    return glm::vec4(uvMin.x, 1.0f - uvMin.y, uvMax.x - uvMin.x, uvMin.y - uvMax.y);
}

void UIRenderer::DrawSprite(
    const Texture2D& texture, const glm::vec2& spritePos, const glm::vec2& spriteSize,
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint)
{
    const glm::vec4 uvRect = SpriteUVRect(spritePos, spriteSize, glm::vec2(texture.GetSize()));
    DrawQuad(position, size, texture, uvRect, tint);
}

void UIRenderer::DrawSpriteInverted(
    const Texture2D& texture, const glm::vec2& spritePos, const glm::vec2& spriteSize,
    const glm::vec2& position, const glm::vec2& size)
{
    const glm::vec4 uvRect = SpriteUVRect(spritePos, spriteSize, glm::vec2(texture.GetSize()));
    DrawQuadInverted(position, size, texture, uvRect);
}

void UIRenderer::DrawSlicedSprite(
    const Texture2D& texture, const glm::vec2& spritePos, const glm::vec2& spriteSize,
    const glm::vec2& position, const glm::vec2& size, const glm::vec4& tint)
{
    const float srcThird = spriteSize.x / 3.0f;
    const float capWidth = glm::floor(srcThird * size.y / spriteSize.y);
    const float midWidth = size.x - 2.0f * capWidth;

    DrawSprite(texture, spritePos, glm::vec2(srcThird, spriteSize.y),
        position, glm::vec2(capWidth, size.y), tint);

    DrawSprite(texture, spritePos + glm::vec2(srcThird, 0.0f), glm::vec2(srcThird, spriteSize.y),
        position + glm::vec2(capWidth, 0.0f), glm::vec2(midWidth, size.y), tint);

    DrawSprite(texture, spritePos + glm::vec2(2.0f * srcThird, 0.0f),
        glm::vec2(spriteSize.x - 2.0f * srcThird, spriteSize.y),
        position + glm::vec2(size.x - capWidth, 0.0f), glm::vec2(capWidth, size.y), tint);
}

}
