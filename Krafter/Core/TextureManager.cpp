#include "vulkan/vulkan.h"

#include "vk_mem_alloc.h"

#include "Krafter/Core/TextureManager.h"
#include "Krafter/Core/VulkanContext.h"

namespace Krafter {

TextureManager::TextureManager(VulkanContext& context, uint32_t framesInFlight)
    : m_Context(context)
{
    m_TrashBins.resize(framesInFlight);

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkCheck(vkCreateSampler(m_Context.GetDevice(), &samplerInfo, nullptr, &m_NearestSampler), "vkCreateSampler");

    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    VkCheck(vkCreateDescriptorSetLayout(m_Context.GetDevice(), &layoutInfo, nullptr, &m_TextureSetLayout),
        "vkCreateDescriptorSetLayout");

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 64;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 64;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    VkCheck(vkCreateDescriptorPool(m_Context.GetDevice(), &poolInfo, nullptr, &m_TexturePool), "vkCreateDescriptorPool");
}

TextureManager::~TextureManager()
{
    for (auto& bin : m_TrashBins) {
        for (const DeferredTexture& texture : bin) {
            vkFreeDescriptorSets(m_Context.GetDevice(), m_TexturePool, 1, &texture.set);
            vkDestroyImageView(m_Context.GetDevice(), texture.view, nullptr);
            vmaDestroyImage(m_Context.GetAllocator(), texture.image, texture.allocation);
        }
    }
    for (const DeferredTexture& texture : m_PendingTrash) {
        vkFreeDescriptorSets(m_Context.GetDevice(), m_TexturePool, 1, &texture.set);
        vkDestroyImageView(m_Context.GetDevice(), texture.view, nullptr);
        vmaDestroyImage(m_Context.GetAllocator(), texture.image, texture.allocation);
    }

    vkDestroySampler(m_Context.GetDevice(), m_NearestSampler, nullptr);
    vkDestroyDescriptorPool(m_Context.GetDevice(), m_TexturePool, nullptr);
    vkDestroyDescriptorSetLayout(m_Context.GetDevice(), m_TextureSetLayout, nullptr);
}

VkDescriptorSet TextureManager::AllocateTextureSet(VkImageView view)
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_TexturePool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_TextureSetLayout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    VkCheck(vkAllocateDescriptorSets(m_Context.GetDevice(), &allocInfo, &set), "vkAllocateDescriptorSets");

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.sampler = m_NearestSampler;
    imageInfo.imageView = view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(m_Context.GetDevice(), 1, &write, 0, nullptr);

    return set;
}

void TextureManager::DestroyTexture(VkImage image, VmaAllocation allocation, VkImageView view, VkDescriptorSet set)
{
    m_PendingTrash.push_back({ image, allocation, view, set });
}

void TextureManager::Recycle(uint32_t frameIndex)
{
    for (const DeferredTexture& texture : m_TrashBins[frameIndex]) {
        vkFreeDescriptorSets(m_Context.GetDevice(), m_TexturePool, 1, &texture.set);
        vkDestroyImageView(m_Context.GetDevice(), texture.view, nullptr);
        vmaDestroyImage(m_Context.GetAllocator(), texture.image, texture.allocation);
    }
    m_TrashBins[frameIndex].clear();
    m_TrashBins[frameIndex].swap(m_PendingTrash);
}

}
