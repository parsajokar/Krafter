#include <algorithm>
#include <cstring>
#include <vector>

#include "vulkan/vulkan.h"

#include "vk_mem_alloc.h"

#include "Krafter/Core/BufferManager.h"
#include "Krafter/Core/VulkanContext.h"

namespace Krafter {

namespace {

void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout from, VkImageLayout to)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = from;
    barrier.newLayout = to;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;
    if (to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = (from == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ? VK_ACCESS_SHADER_READ_BIT : 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = (from == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
            : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

}

BufferManager::BufferManager(VulkanContext& context, uint32_t framesInFlight)
    : m_Context(context)
{
    m_TrashBins.resize(framesInFlight);
}

BufferManager::~BufferManager()
{
    for (auto& bin : m_TrashBins) {
        for (const GpuBuffer& buffer : bin) {
            vmaDestroyBuffer(m_Context.GetAllocator(), buffer.buffer, buffer.allocation);
        }
    }
    for (const GpuBuffer& buffer : m_PendingTrash) {
        vmaDestroyBuffer(m_Context.GetAllocator(), buffer.buffer, buffer.allocation);
    }
    for (const GpuBuffer& buffer : m_ReadyToFree) {
        vmaDestroyBuffer(m_Context.GetAllocator(), buffer.buffer, buffer.allocation);
    }
    for (const PendingBufferUpload& upload : m_PendingBufferUploads) {
        vmaDestroyBuffer(m_Context.GetAllocator(), upload.staging.buffer, upload.staging.allocation);
    }
    for (const PendingImageUpload& upload : m_PendingImageUploads) {
        vmaDestroyBuffer(m_Context.GetAllocator(), upload.staging.buffer, upload.staging.allocation);
    }
}

GpuBuffer BufferManager::CreateDeviceLocalBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo stagingInfo = {};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = size;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAlloc = {};
    stagingAlloc.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAlloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    GpuBuffer staging;
    VmaAllocationInfo stagingInfoOut = {};
    VkCheck(vmaCreateBuffer(m_Context.GetAllocator(), &stagingInfo, &stagingAlloc, &staging.buffer, &staging.allocation, &stagingInfoOut),
        "vmaCreateBuffer (staging)");
    std::memcpy(stagingInfoOut.pMappedData, data, static_cast<size_t>(size));
    vmaFlushAllocation(m_Context.GetAllocator(), staging.allocation, 0, size);

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo deviceAlloc = {};
    deviceAlloc.usage = VMA_MEMORY_USAGE_AUTO;

    GpuBuffer result;
    VkCheck(vmaCreateBuffer(m_Context.GetAllocator(), &bufferInfo, &deviceAlloc, &result.buffer, &result.allocation, nullptr),
        "vmaCreateBuffer (device)");

    {
        std::lock_guard<std::mutex> lock(m_UploadMutex);
        m_PendingBufferUploads.push_back({ staging, result.buffer, size });
    }
    return result;
}

GpuBuffer BufferManager::CreateHostVisibleBuffer(VkDeviceSize size, VkBufferUsageFlags usage, void** mappedOut)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    GpuBuffer buffer;
    VmaAllocationInfo info = {};
    VkCheck(vmaCreateBuffer(m_Context.GetAllocator(), &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &info),
        "vmaCreateBuffer (host visible)");
    *mappedOut = info.pMappedData;
    return buffer;
}

void BufferManager::QueueImageUpload(VkImage image, VkImageLayout oldLayout,
    int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels)
{
    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;

    VkBufferCreateInfo stagingInfo = {};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = size;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAlloc = {};
    stagingAlloc.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAlloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    GpuBuffer staging;
    VmaAllocationInfo stagingInfoOut = {};
    VkCheck(vmaCreateBuffer(m_Context.GetAllocator(), &stagingInfo, &stagingAlloc, &staging.buffer, &staging.allocation, &stagingInfoOut),
        "vmaCreateBuffer (image staging)");
    std::memcpy(stagingInfoOut.pMappedData, pixels, static_cast<size_t>(size));
    vmaFlushAllocation(m_Context.GetAllocator(), staging.allocation, 0, size);

    m_PendingImageUploads.push_back({ staging, image, oldLayout, x, y, width, height });
}

void BufferManager::RecordUploads(VkCommandBuffer cmd)
{
    std::vector<PendingBufferUpload> bufferUploads;
    {
        std::lock_guard<std::mutex> lock(m_UploadMutex);
        bufferUploads.swap(m_PendingBufferUploads);
    }

    if (bufferUploads.empty() && m_PendingImageUploads.empty()) {
        return;
    }

    for (const PendingBufferUpload& upload : bufferUploads) {
        VkBufferCopy copy = {};
        copy.size = upload.size;
        vkCmdCopyBuffer(cmd, upload.staging.buffer, upload.dst, 1, &copy);
        m_PendingTrash.push_back(upload.staging);
    }

    for (const PendingImageUpload& upload : m_PendingImageUploads) {
        TransitionImageLayout(cmd, upload.dst, upload.oldLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy copy = {};
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageOffset = { upload.x, upload.y, 0 };
        copy.imageExtent = { static_cast<uint32_t>(upload.width), static_cast<uint32_t>(upload.height), 1 };
        vkCmdCopyBufferToImage(cmd, upload.staging.buffer, upload.dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        TransitionImageLayout(cmd, upload.dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_PendingTrash.push_back(upload.staging);
    }

    if (!bufferUploads.empty()) {
        VkMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            0, 1, &barrier, 0, nullptr, 0, nullptr);
    }

    m_PendingImageUploads.clear();
}

void BufferManager::DestroyBuffer(const GpuBuffer& buffer)
{
    if (buffer.buffer != VK_NULL_HANDLE) {
        m_PendingTrash.push_back(buffer);
    }
}

void BufferManager::FlushHostBuffer(const GpuBuffer& buffer, VkDeviceSize offset, VkDeviceSize size)
{
    vmaFlushAllocation(m_Context.GetAllocator(), buffer.allocation, offset, size);
}

void BufferManager::Recycle(uint32_t frameIndex)
{
    for (const GpuBuffer& buffer : m_TrashBins[frameIndex]) {
        m_ReadyToFree.push_back(buffer);
    }
    m_TrashBins[frameIndex].clear();
    m_TrashBins[frameIndex].swap(m_PendingTrash);

    const size_t budget = std::max<size_t>(k_MaxFreesPerFrame, m_ReadyToFree.size() / 4);
    for (size_t freed = 0; freed < budget && !m_ReadyToFree.empty(); freed++) {
        const GpuBuffer& buffer = m_ReadyToFree.front();
        vmaDestroyBuffer(m_Context.GetAllocator(), buffer.buffer, buffer.allocation);
        m_ReadyToFree.pop_front();
    }
}

}
