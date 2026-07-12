#pragma once

#include <cstdint>

#include "vulkan/vulkan.h"

typedef struct VmaAllocator_T* VmaAllocator;

namespace Krafter {

class Window;

void VkCheck(VkResult result, const char* what);

class VulkanContext {
public:
    explicit VulkanContext(Window& window);
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    void WaitIdle() const;

    VkInstance GetInstance() const { return m_Instance; }
    VkSurfaceKHR GetSurface() const { return m_Surface; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkDevice GetDevice() const { return m_Device; }
    uint32_t GetGraphicsQueueFamily() const { return m_GraphicsFamily; }
    uint32_t GetPresentQueueFamily() const { return m_PresentFamily; }
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    VkQueue GetPresentQueue() const { return m_PresentQueue; }
    VmaAllocator GetAllocator() const { return m_Allocator; }

    VkFormat FindDepthFormat() const;

private:
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateAllocator();

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);

    Window& m_Window;

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;

    uint32_t m_GraphicsFamily = 0;
    uint32_t m_PresentFamily = 0;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;

    VmaAllocator m_Allocator = VK_NULL_HANDLE;
};

}
