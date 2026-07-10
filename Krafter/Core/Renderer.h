#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "vulkan/vulkan.h"

#include "glm/glm.hpp"

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace Krafter {

class Window;

struct GpuBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
};

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

    VkInstance GetInstance() const { return m_Instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkDevice GetDevice() const { return m_Device; }
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    uint32_t GetGraphicsQueueFamily() const { return m_GraphicsFamily; }
    VkRenderPass GetRenderPass() const { return m_RenderPass; }
    VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffers[m_CurrentFrame]; }
    VkExtent2D GetSwapchainExtent() const { return m_SwapchainExtent; }
    uint32_t GetSwapchainImageCount() const { return static_cast<uint32_t>(m_SwapchainImages.size()); }
    uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
    VmaAllocator GetAllocator() const { return m_Allocator; }
    VkSampler GetNearestSampler() const { return m_NearestSampler; }
    VkDescriptorSetLayout GetTextureSetLayout() const { return m_TextureSetLayout; }

    GpuBuffer CreateDeviceLocalBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage);

    GpuBuffer CreateHostVisibleBuffer(VkDeviceSize size, VkBufferUsageFlags usage, void** mappedOut);

    void DestroyBuffer(const GpuBuffer& buffer);

    void FlushHostBuffer(const GpuBuffer& buffer, VkDeviceSize offset, VkDeviceSize size);

    VkDescriptorSet AllocateTextureSet(VkImageView view);

    void DestroyTexture(VkImage image, VmaAllocation allocation, VkImageView view, VkDescriptorSet set);

    void QueueImageUpload(VkImage image, VkImageLayout oldLayout,
        int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels);

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer cmd);

private:
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateAllocator();

    void CreateSwapchain();
    void CreateImageViews();
    void CreateDepthResources();
    void CreateRenderPass();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void CreateTextureInfrastructure();

    void RecordPendingUploads(VkCommandBuffer cmd);

    VkFormat FindDepthFormat() const;

    void CleanupSwapchain();
    void RecreateSwapchain();

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);

    inline static Renderer* s_Instance = nullptr;

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

    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_SwapchainExtent = { 0, 0 };
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;

    VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
    VkImage m_DepthImage = VK_NULL_HANDLE;
    VmaAllocation m_DepthAllocation = VK_NULL_HANDLE;
    VkImageView m_DepthView = VK_NULL_HANDLE;

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    VkSampler m_NearestSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_TextureSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_TexturePool = VK_NULL_HANDLE;

    std::vector<GpuBuffer> m_PendingTrash;
    std::array<std::vector<GpuBuffer>, k_MaxFramesInFlight> m_TrashBins;

    static constexpr uint32_t k_MaxFreesPerFrame = 8;
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

    struct DeferredTexture {
        VkImage image;
        VmaAllocation allocation;
        VkImageView view;
        VkDescriptorSet set;
    };
    std::vector<DeferredTexture> m_PendingTextureTrash;
    std::array<std::vector<DeferredTexture>, k_MaxFramesInFlight> m_TextureTrashBins;

    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    std::vector<VkFence> m_ImagesInFlight;

    uint32_t m_CurrentFrame = 0;
    uint32_t m_ImageIndex = 0;

    VkClearValue m_ClearColor = { { { 0.470f, 0.655f, 1.0f, 1.0f } } };
};

}
