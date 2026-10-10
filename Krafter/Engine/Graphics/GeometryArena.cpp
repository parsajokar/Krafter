#include <algorithm>

#include "vk_mem_alloc.h"

#include "Krafter/Engine/Graphics/GeometryArena.h"
#include "Krafter/Engine/Graphics/VulkanContext.h"

namespace Krafter {

GeometryArena::GeometryArena(VulkanContext& context, BufferManager& buffers, uint32_t framesInFlight)
    : m_Context(context)
    , m_Buffers(buffers)
{
    m_FreeBins.resize(framesInFlight);
}

GeometryArena::~GeometryArena()
{
    for (Page& page : m_Pages) {
        vmaClearVirtualBlock(page.block);
        vmaDestroyVirtualBlock(page.block);
        vmaDestroyBuffer(m_Context.GetAllocator(), page.buffer.buffer, page.buffer.allocation);
    }
}

void GeometryArena::AddPage(VkDeviceSize size)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    Page page;
    page.size = size;
    VkCheck(vmaCreateBuffer(m_Context.GetAllocator(), &bufferInfo, &allocInfo, &page.buffer.buffer, &page.buffer.allocation, nullptr),
        "vmaCreateBuffer (geometry page)");

    VmaVirtualBlockCreateInfo blockInfo = {};
    blockInfo.size = size;
    VkCheck(vmaCreateVirtualBlock(&blockInfo, &page.block), "vmaCreateVirtualBlock");

    m_Pages.push_back(page);
}

GeometryAllocation GeometryArena::Allocate(VkDeviceSize size, VkDeviceSize alignment)
{
    VmaVirtualAllocationCreateInfo request = {};
    request.size = size;
    request.alignment = alignment;

    GeometryAllocation result;
    result.size = size;
    for (uint32_t i = 0; i < m_Pages.size(); i++) {
        if (vmaVirtualAllocate(m_Pages[i].block, &request, &result.handle, &result.offset) == VK_SUCCESS) {
            result.page = i;
            return result;
        }
    }

    AddPage(std::max(k_PageSize, size));
    const uint32_t page = static_cast<uint32_t>(m_Pages.size() - 1);
    VkCheck(vmaVirtualAllocate(m_Pages[page].block, &request, &result.handle, &result.offset), "vmaVirtualAllocate");
    result.page = page;
    return result;
}

void GeometryArena::Upload(const GeometryAllocation& allocation, VkDeviceSize offset, const void* data, VkDeviceSize size)
{
    m_Buffers.QueueBufferUpload(GetBuffer(allocation.page), allocation.offset + offset, data, size);
}

void GeometryArena::Free(const GeometryAllocation& allocation)
{
    if (allocation.IsValid()) {
        m_PendingFrees.push_back(allocation);
    }
}

void GeometryArena::FreeNow(const GeometryAllocation& allocation)
{
    vmaVirtualFree(m_Pages[allocation.page].block, allocation.handle);
}

void GeometryArena::Recycle(uint32_t frameIndex)
{
    for (const GeometryAllocation& allocation : m_FreeBins[frameIndex]) {
        FreeNow(allocation);
    }
    m_FreeBins[frameIndex].clear();
    m_FreeBins[frameIndex].swap(m_PendingFrees);
}

VkDeviceSize GeometryArena::GetUsedBytes() const
{
    VkDeviceSize used = 0;
    for (const Page& page : m_Pages) {
        VmaStatistics stats = {};
        vmaGetVirtualBlockStatistics(page.block, &stats);
        used += stats.allocationBytes;
    }
    return used;
}

VkDeviceSize GeometryArena::GetCapacityBytes() const
{
    VkDeviceSize capacity = 0;
    for (const Page& page : m_Pages) {
        capacity += page.size;
    }
    return capacity;
}

}
