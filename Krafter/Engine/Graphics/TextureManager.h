#pragma once

#include <cstdint>
#include <vector>

#include "vulkan/vulkan.h"

typedef struct VmaAllocation_T* VmaAllocation;

namespace Krafter {

class VulkanContext;

class TextureManager {
public:
    TextureManager(VulkanContext& context, uint32_t framesInFlight);
    ~TextureManager();

    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    VkSampler GetNearestSampler() const { return m_NearestSampler; }
    VkDescriptorSetLayout GetTextureSetLayout() const { return m_TextureSetLayout; }

    VkDescriptorSet AllocateTextureSet(VkImageView view);

    void DestroyTexture(VkImage image, VmaAllocation allocation, VkImageView view, VkDescriptorSet set);

    void Recycle(uint32_t frameIndex);

private:
    VulkanContext& m_Context;

    VkSampler m_NearestSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_TextureSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_TexturePool = VK_NULL_HANDLE;

    struct DeferredTexture {
        VkImage image;
        VmaAllocation allocation;
        VkImageView view;
        VkDescriptorSet set;
    };
    std::vector<DeferredTexture> m_PendingTrash;
    std::vector<std::vector<DeferredTexture>> m_TrashBins;
};

}
