#pragma once

#include <cstdint>
#include <vector>

#include "vulkan/vulkan.h"

#include "glm/glm.hpp"

#include "Krafter/Core/BufferManager.h"
#include "Krafter/Core/Swapchain.h"
#include "Krafter/Core/TextureManager.h"
#include "Krafter/Core/VulkanContext.h"

namespace Krafter {

class Window;

class Renderer {
public:
    static constexpr uint32_t k_MaxFramesInFlight = 2;

    Renderer(Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    static Renderer& Get() { return *s_Instance; }

    void SetClearColor(const glm::vec3& color);

    void WaitIdle();

    bool BeginFrame();
    void EndFrame();

    void RenderImGui();

    VkInstance GetInstance() const { return m_Context.GetInstance(); }
    VkPhysicalDevice GetPhysicalDevice() const { return m_Context.GetPhysicalDevice(); }
    VkDevice GetDevice() const { return m_Context.GetDevice(); }
    VkQueue GetGraphicsQueue() const { return m_Context.GetGraphicsQueue(); }
    uint32_t GetGraphicsQueueFamily() const { return m_Context.GetGraphicsQueueFamily(); }
    VkRenderPass GetRenderPass() const { return m_Swapchain.GetRenderPass(); }
    VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffers[m_CurrentFrame]; }
    VkExtent2D GetSwapchainExtent() const { return m_Swapchain.GetExtent(); }
    uint32_t GetSwapchainImageCount() const { return m_Swapchain.GetImageCount(); }
    uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
    VmaAllocator GetAllocator() const { return m_Context.GetAllocator(); }
    VkSampler GetNearestSampler() const { return m_Textures.GetNearestSampler(); }
    VkDescriptorSetLayout GetTextureSetLayout() const { return m_Textures.GetTextureSetLayout(); }

    GpuBuffer CreateDeviceLocalBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage)
    {
        return m_Buffers.CreateDeviceLocalBuffer(data, size, usage);
    }

    GpuBuffer CreateHostVisibleBuffer(VkDeviceSize size, VkBufferUsageFlags usage, void** mappedOut)
    {
        return m_Buffers.CreateHostVisibleBuffer(size, usage, mappedOut);
    }

    void DestroyBuffer(const GpuBuffer& buffer)
    {
        m_Buffers.DestroyBuffer(buffer);
    }

    void FlushHostBuffer(const GpuBuffer& buffer, VkDeviceSize offset, VkDeviceSize size)
    {
        m_Buffers.FlushHostBuffer(buffer, offset, size);
    }

    void QueueImageUpload(VkImage image, VkImageLayout oldLayout,
        int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels)
    {
        m_Buffers.QueueImageUpload(image, oldLayout, x, y, width, height, pixels);
    }

    VkDescriptorSet AllocateTextureSet(VkImageView view)
    {
        return m_Textures.AllocateTextureSet(view);
    }

    void DestroyTexture(VkImage image, VmaAllocation allocation, VkImageView view, VkDescriptorSet set)
    {
        m_Textures.DestroyTexture(image, allocation, view, set);
    }

private:
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();

    void RecreateSwapchain();

    inline static Renderer* s_Instance = nullptr;

    VulkanContext m_Context;
    Swapchain m_Swapchain;
    BufferManager m_Buffers;
    TextureManager m_Textures;

    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    std::vector<VkFence> m_ImagesInFlight;

    uint32_t m_CurrentFrame = 0;
    uint32_t m_ImageIndex = 0;

    VkClearValue m_ClearColor = { { { 0.470f, 0.655f, 1.0f, 1.0f } } };
};

}
