#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "vulkan/vulkan.h"

typedef struct VmaAllocation_T* VmaAllocation;

namespace Krafter {

class VulkanContext;

struct GpuBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
};

class BufferManager {
public:
    BufferManager(VulkanContext& context, uint32_t framesInFlight);
    ~BufferManager();

    BufferManager(const BufferManager&) = delete;
    BufferManager& operator=(const BufferManager&) = delete;

    GpuBuffer CreateDeviceLocalBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage);

    GpuBuffer CreateHostVisibleBuffer(VkDeviceSize size, VkBufferUsageFlags usage, void** mappedOut);

    void DestroyBuffer(const GpuBuffer& buffer);

    void FlushHostBuffer(const GpuBuffer& buffer, VkDeviceSize offset, VkDeviceSize size);

    void QueueImageUpload(VkImage image, VkImageLayout oldLayout,
        int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels);

    void RecordUploads(VkCommandBuffer cmd);

    void Recycle(uint32_t frameIndex);

private:
    static constexpr uint32_t k_MaxFreesPerFrame = 8;

    VulkanContext& m_Context;

    std::vector<GpuBuffer> m_PendingTrash;
    std::vector<std::vector<GpuBuffer>> m_TrashBins;
    std::deque<GpuBuffer> m_ReadyToFree;

    struct PendingBufferUpload {
        GpuBuffer staging;
        VkBuffer dst;
        VkDeviceSize size;
    };
    struct PendingImageUpload {
        GpuBuffer staging;
        VkImage dst;
        VkImageLayout oldLayout;
        int32_t x, y, width, height;
    };
    std::vector<PendingBufferUpload> m_PendingBufferUploads;
    std::vector<PendingImageUpload> m_PendingImageUploads;
    std::mutex m_UploadMutex;
};

}
