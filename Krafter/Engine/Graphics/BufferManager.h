#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "vulkan/vulkan.h"

typedef struct VmaAllocation_T* VmaAllocation;

namespace Krafter {

class VulkanContext;
class BufferManager;

struct GpuBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
};

class OwnedBuffer {
public:
    OwnedBuffer() = default;
    OwnedBuffer(BufferManager& manager, GpuBuffer buffer)
        : m_Manager(&manager)
        , m_Buffer(buffer)
    {
    }
    ~OwnedBuffer()
    {
        Reset();
    }

    OwnedBuffer(const OwnedBuffer&) = delete;
    OwnedBuffer& operator=(const OwnedBuffer&) = delete;

    OwnedBuffer(OwnedBuffer&& other) noexcept
        : m_Manager(other.m_Manager)
        , m_Buffer(other.m_Buffer)
    {
        other.m_Manager = nullptr;
        other.m_Buffer = {};
    }

    OwnedBuffer& operator=(OwnedBuffer&& other) noexcept
    {
        if (this != &other) {
            Reset();
            m_Manager = other.m_Manager;
            m_Buffer = other.m_Buffer;
            other.m_Manager = nullptr;
            other.m_Buffer = {};
        }
        return *this;
    }

    const GpuBuffer& Get() const
    {
        return m_Buffer;
    }

    VkBuffer GetHandle() const
    {
        return m_Buffer.buffer;
    }

    void Reset();

private:
    BufferManager* m_Manager = nullptr;
    GpuBuffer m_Buffer;
};

class BufferManager {
public:
    BufferManager(VulkanContext& context, uint32_t framesInFlight);
    ~BufferManager();

    BufferManager(const BufferManager&) = delete;
    BufferManager& operator=(const BufferManager&) = delete;

    OwnedBuffer CreateDeviceLocalBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage);

    void QueueBufferUpload(VkBuffer dst, VkDeviceSize dstOffset, const void* data, VkDeviceSize size);

    OwnedBuffer CreateHostVisibleBuffer(VkDeviceSize size, VkBufferUsageFlags usage, void** mappedOut);

    void FlushHostBuffer(const OwnedBuffer& buffer, VkDeviceSize offset, VkDeviceSize size);

    void QueueImageUpload(VkImage image, VkImageLayout oldLayout,
        int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels);

    void RecordUploads(VkCommandBuffer cmd);

    void Recycle(uint32_t frameIndex);

private:
    friend class OwnedBuffer;

    void DestroyBuffer(const GpuBuffer& buffer);

    static constexpr uint32_t k_MaxFreesPerFrame = 8;

    VulkanContext& m_Context;

    std::vector<GpuBuffer> m_PendingTrash;
    std::vector<std::vector<GpuBuffer>> m_TrashBins;
    std::deque<GpuBuffer> m_ReadyToFree;

    struct PendingBufferUpload {
        GpuBuffer staging;
        VkBuffer dst;
        VkDeviceSize dstOffset;
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
