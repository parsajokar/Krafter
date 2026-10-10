#pragma once

#include <cstdint>
#include <vector>

#include "vulkan/vulkan.h"

#include "Krafter/Engine/Graphics/BufferManager.h"

typedef struct VmaVirtualBlock_T* VmaVirtualBlock;
typedef struct VmaVirtualAllocation_T* VmaVirtualAllocation;

namespace Krafter {

class VulkanContext;

struct GeometryAllocation {
    uint32_t page = UINT32_MAX;
    VkDeviceSize offset = 0;
    VkDeviceSize size = 0;
    VmaVirtualAllocation handle = VK_NULL_HANDLE;

    bool IsValid() const
    {
        return page != UINT32_MAX;
    }
};

class GeometryArena {
public:
    static constexpr VkDeviceSize k_PageSize = 64ull * 1024 * 1024;

    GeometryArena(VulkanContext& context, BufferManager& buffers, uint32_t framesInFlight);
    ~GeometryArena();

    GeometryArena(const GeometryArena&) = delete;
    GeometryArena& operator=(const GeometryArena&) = delete;

    GeometryAllocation Allocate(VkDeviceSize size, VkDeviceSize alignment);

    void Upload(const GeometryAllocation& allocation, VkDeviceSize offset, const void* data, VkDeviceSize size);

    void Free(const GeometryAllocation& allocation);

    void Recycle(uint32_t frameIndex);

    VkBuffer GetBuffer(uint32_t page) const
    {
        return m_Pages[page].buffer.buffer;
    }

    size_t GetPageCount() const
    {
        return m_Pages.size();
    }

    VkDeviceSize GetUsedBytes() const;
    VkDeviceSize GetCapacityBytes() const;

private:
    struct Page {
        GpuBuffer buffer;
        VmaVirtualBlock block = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
    };

    void AddPage(VkDeviceSize size);
    void FreeNow(const GeometryAllocation& allocation);

    VulkanContext& m_Context;
    BufferManager& m_Buffers;

    std::vector<Page> m_Pages;

    std::vector<GeometryAllocation> m_PendingFrees;
    std::vector<std::vector<GeometryAllocation>> m_FreeBins;
};

}
