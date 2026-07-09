#include <cassert>
#include <iostream>

#include "vulkan/vulkan.h"

#include "vk_mem_alloc.h"

#include "stb_image.h"

#include "Krafter/Core/Renderer.h"
#include "Krafter/Renderer/Texture.h"

namespace Krafter {

Texture2D::Texture2D(std::string_view path)
{
    stbi_set_flip_vertically_on_load(true);

    int32_t channelsInFile = 0;
    // Force four channels so the upload is always tightly packed RGBA8.
    uint8_t* data = stbi_load(path.data(), &m_Size.x, &m_Size.y, &channelsInFile, STBI_rgb_alpha);
    if (!data) {
        std::cerr << "[FILE] Could not read " << path << std::endl;
        assert(false);
    }

    Create(data, m_Size.x, m_Size.y);

    stbi_image_free(data);
}

Texture2D::Texture2D(const uint8_t* pixels, int32_t width, int32_t height)
{
    m_Size = glm::ivec2(width, height);
    Create(pixels, width, height);
}

void Texture2D::Create(const void* pixels, int32_t width, int32_t height)
{
    Renderer& renderer = Renderer::Get();

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateImage(renderer.GetAllocator(), &imageInfo, &allocInfo, &m_Image, &m_Allocation, nullptr);

    // Queue the pixel upload; it is recorded into the next frame's command buffer.
    // Nothing samples the texture before then, so its initial undefined contents
    // are never observed.
    renderer.QueueImageUpload(m_Image, VK_IMAGE_LAYOUT_UNDEFINED, 0, 0, width, height, pixels);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(renderer.GetDevice(), &viewInfo, nullptr, &m_View);

    m_DescriptorSet = renderer.AllocateTextureSet(m_View);
}

Texture2D::~Texture2D()
{
    Renderer::Get().DestroyTexture(m_Image, m_Allocation, m_View, m_DescriptorSet);
}

void Texture2D::UpdateRegion(int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels) const
{
    // The atlas is in shader-read layout after the frame that first used it; the
    // renderer transitions, copies, and transitions back when it records this.
    Renderer::Get().QueueImageUpload(m_Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, x, y, width, height, pixels);
}

} // namespace Krafter
