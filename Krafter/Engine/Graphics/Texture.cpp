#include "vulkan/vulkan.h"

#include "vk_mem_alloc.h"

#include "Krafter/Engine/Graphics/Image.h"
#include "Krafter/Engine/Graphics/Renderer.h"
#include "Krafter/Engine/Graphics/Texture.h"

namespace Krafter {

Texture2D::Texture2D(Renderer& renderer, const std::string& path)
    : m_Renderer(renderer)
{
    const Image image = LoadImage(path, true);
    if (!image) {
        const uint8_t magenta[4] = { 255, 0, 255, 255 };
        m_Size = glm::ivec2(1, 1);
        Create(magenta, 1, 1);
        return;
    }

    m_Size = glm::ivec2(image.width, image.height);
    Create(image.pixels.get(), image.width, image.height);
}

Texture2D::Texture2D(Renderer& renderer, const uint8_t* pixels, int32_t width, int32_t height)
    : m_Renderer(renderer)
{
    m_Size = glm::ivec2(width, height);
    Create(pixels, width, height);
}

void Texture2D::Create(const void* pixels, int32_t width, int32_t height)
{
    Renderer& renderer = m_Renderer;

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
    m_Renderer.DestroyTexture(m_Image, m_Allocation, m_View, m_DescriptorSet);
}

void Texture2D::UpdateRegion(int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels)
{
    m_Renderer.QueueImageUpload(m_Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, x, y, width, height, pixels);
}

}
