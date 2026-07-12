#pragma once

#include <cstdint>
#include <vector>

#include "vulkan/vulkan.h"

typedef struct VmaAllocation_T* VmaAllocation;

namespace Krafter {

class VulkanContext;
class Window;

class Swapchain {
public:
    Swapchain(VulkanContext& context, Window& window);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    void Recreate();

    VkSwapchainKHR GetHandle() const { return m_Swapchain; }
    VkRenderPass GetRenderPass() const { return m_RenderPass; }
    VkExtent2D GetExtent() const { return m_Extent; }
    VkFormat GetImageFormat() const { return m_Format; }
    uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }
    VkFramebuffer GetFramebuffer(uint32_t index) const { return m_Framebuffers[index]; }

private:
    void CreateSwapchain();
    void CreateImageViews();
    void CreateDepthResources();
    void CreateRenderPass();
    void CreateFramebuffers();
    void Cleanup();

    VulkanContext& m_Context;
    Window& m_Window;

    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkFormat m_Format = VK_FORMAT_UNDEFINED;
    VkExtent2D m_Extent = { 0, 0 };
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;

    VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
    VkImage m_DepthImage = VK_NULL_HANDLE;
    VmaAllocation m_DepthAllocation = VK_NULL_HANDLE;
    VkImageView m_DepthView = VK_NULL_HANDLE;

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
};

}
