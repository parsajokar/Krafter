#pragma once

#include <cstdint>
#include <string>

#include "vulkan/vulkan.h"

#include "glm/glm.hpp"

typedef struct VmaAllocation_T* VmaAllocation;

namespace Krafter {

class Renderer;

class Texture2D {
public:
    Texture2D(Renderer& renderer, const std::string& path);
    Texture2D(Renderer& renderer, const uint8_t* pixels, int32_t width, int32_t height);
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

    void UpdateRegion(int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels);

    inline const glm::ivec2& GetSize() const
    {
        return m_Size;
    }

private:
    void Create(const void* pixels, int32_t width, int32_t height);

    Renderer& m_Renderer;

    glm::ivec2 m_Size;

    VkImage m_Image = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = VK_NULL_HANDLE;
    VkImageView m_View = VK_NULL_HANDLE;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
};

}
