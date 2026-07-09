#pragma once

#include <cstdint>
#include <string_view>

#include "vulkan/vulkan.h"

#include "glm/glm.hpp"

typedef struct VmaAllocation_T* VmaAllocation;

namespace Krafter {

// A 2D texture backed by a device-local VkImage, with a ready-to-bind descriptor
// set (set 0: one combined image sampler) matching the shaders' texture binding.
class Texture2D {
public:
    Texture2D(std::string_view path);
    // Builds a texture directly from tightly-packed RGBA8 pixels (e.g. a solid
    // colour), for cases with no image file.
    Texture2D(const uint8_t* pixels, int32_t width, int32_t height);
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    // The descriptor set to bind for set 0 when sampling this texture.
    VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

    // No-op shim kept only so the not-yet-ported (GL) UIRenderer still compiles;
    // Vulkan binds textures through descriptor sets, not texture units. Removed
    // when UIRenderer moves to Vulkan.
    void Bind(uint32_t) const { }

    // Overwrites a rectangular region with tightly-packed RGBA8 pixels (used by the
    // animated water and lava tiles).
    void UpdateRegion(int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels) const;

    inline const glm::ivec2& GetSize() const
    {
        return m_Size;
    }

private:
    // Creates the image, view, and descriptor set, and queues the pixel upload.
    void Create(const void* pixels, int32_t width, int32_t height);

    glm::ivec2 m_Size;

    VkImage m_Image = VK_NULL_HANDLE;
    VmaAllocation m_Allocation = VK_NULL_HANDLE;
    VkImageView m_View = VK_NULL_HANDLE;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
};

} // namespace Krafter
