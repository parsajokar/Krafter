#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

#include "vulkan/vulkan.h"

#include "glm/glm.hpp"

// Forward-declared so the heavy vk_mem_alloc.h header stays out of this widely
// included file; only Renderer.cpp pulls in the full VMA implementation header.
typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace Krafter {

class Window;

// A GPU buffer and the VMA allocation backing it, freed together.
struct GpuBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
};

// The Vulkan graphics device: owns the instance, device, swapchain, render pass,
// and the per-frame command buffer and synchronisation. It knows nothing about
// what is being drawn; concrete renderers (WorldRenderer, UIRenderer) record
// their draw calls into the command buffer BeginFrame opens.
class Renderer {
public:
    // Frames the CPU may work on before waiting for the GPU. Two lets the CPU
    // record the next frame while the GPU finishes the current one.
    static constexpr uint32_t k_MaxFramesInFlight = 2;

    Renderer(Window& window);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // The single live renderer. Resource classes (Texture2D, ChunkMesh) reach the
    // device through this rather than threading a Renderer& through every layer.
    static Renderer& Get() { return *s_Instance; }

    void SetClearColor(const glm::vec3& color);

    // Blocks until the GPU is idle. Call before destroying resources that in-flight
    // frames may still reference (e.g. when tearing down layers at shutdown).
    void WaitIdle();

    // Frame lifecycle. BeginFrame waits for the current frame's fence, acquires a
    // swapchain image, and begins the primary command buffer and render pass
    // (clearing to the clear colour). It returns false when the swapchain was out
    // of date and had to be recreated, in which case the frame should be skipped.
    // EndFrame ends the render pass, submits the command buffer, and presents.
    bool BeginFrame();
    void EndFrame();

    void RenderImGui();

    // --- Accessors for concrete renderers (used from later milestones) ---
    VkInstance GetInstance() const { return m_Instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkDevice GetDevice() const { return m_Device; }
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    uint32_t GetGraphicsQueueFamily() const { return m_GraphicsFamily; }
    VkRenderPass GetRenderPass() const { return m_RenderPass; }
    VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffers[m_CurrentFrame]; }
    VkExtent2D GetSwapchainExtent() const { return m_SwapchainExtent; }
    uint32_t GetSwapchainImageCount() const { return static_cast<uint32_t>(m_SwapchainImages.size()); }
    // The frame-in-flight slot currently being recorded; used to index per-frame
    // streaming buffers (the UI's dynamic vertex ring).
    uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
    VmaAllocator GetAllocator() const { return m_Allocator; }
    VkSampler GetNearestSampler() const { return m_NearestSampler; }
    VkDescriptorSetLayout GetTextureSetLayout() const { return m_TextureSetLayout; }

    // --- GPU resource helpers (used by Texture2D, ChunkMesh, pipelines) ---

    // Creates a device-local buffer filled with `data` via a staging copy. Used
    // for immutable vertex/index data.
    GpuBuffer CreateDeviceLocalBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage);

    // Creates a persistently-mapped host-visible buffer for per-frame streaming data
    // (the UI's dynamic vertices); the mapped pointer is returned via mappedOut.
    GpuBuffer CreateHostVisibleBuffer(VkDeviceSize size, VkBufferUsageFlags usage, void** mappedOut);

    void DestroyBuffer(const GpuBuffer& buffer);

    // Flushes a range of a host-visible buffer's mapped memory so writes are
    // visible to the GPU. A no-op when the allocation is host-coherent (the usual
    // case on desktop), so callers can flush unconditionally after writing.
    void FlushHostBuffer(const GpuBuffer& buffer, VkDeviceSize offset, VkDeviceSize size);

    // Allocates and writes a descriptor set (set 0, one combined image sampler)
    // pointing at `view` through the shared nearest sampler.
    VkDescriptorSet AllocateTextureSet(VkImageView view);

    // Destroys a texture's image, view, and descriptor set. Deferred like buffers,
    // since an in-flight frame may still sample it (e.g. the world atlas when the
    // scene changes back to the menu).
    void DestroyTexture(VkImage image, VmaAllocation allocation, VkImageView view, VkDescriptorSet set);

    // Queues an upload of `pixels` (RGBA8) into a region of `image`, recorded into
    // the next frame's command buffer rather than submitted immediately, so it
    // never stalls the queue. `oldLayout` is the image's layout on entry.
    void QueueImageUpload(VkImage image, VkImageLayout oldLayout,
        int32_t x, int32_t y, int32_t width, int32_t height, const void* pixels);

    // Records into a throwaway primary command buffer, then submits and waits.
    // Used for uploads and image layout transitions outside the frame loop.
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

    // Records all queued buffer and image uploads into `cmd`, with a barrier so the
    // copies are visible before the render pass reads them.
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

    // Shared depth buffer for the whole swapchain, recreated on resize.
    VkFormat m_DepthFormat = VK_FORMAT_UNDEFINED;
    VkImage m_DepthImage = VK_NULL_HANDLE;
    VmaAllocation m_DepthAllocation = VK_NULL_HANDLE;
    VkImageView m_DepthView = VK_NULL_HANDLE;

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    // Shared texturing state: a nearest sampler (blocks are pixel art), the layout
    // for a one-sampler descriptor set, and the pool those sets are allocated from.
    VkSampler m_NearestSampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_TextureSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_TexturePool = VK_NULL_HANDLE;

    // Deferred buffer deletion: a buffer freed while the GPU may still be reading it
    // (a chunk streaming out of range) is parked here and destroyed only once its
    // frames have finished. Retirements collect in the pending list, are moved into
    // the current frame slot's bin at BeginFrame, and freed two frames later.
    std::vector<GpuBuffer> m_PendingTrash;
    std::array<std::vector<GpuBuffer>, k_MaxFramesInFlight> m_TrashBins;

    // Buffers whose GPU-safety window has passed and are ready to actually free.
    // Freed oldest-first, a bounded number per frame, so a large streaming burst
    // (flying fast in spectator) doesn't stall the main thread destroying them all
    // at once. A deque so draining from the front stays O(1); the budget scales
    // with the backlog so a sustained burst can't grow the list without bound.
    static constexpr uint32_t k_MaxFreesPerFrame = 8;
    std::deque<GpuBuffer> m_ReadyToFree;

    // Queued uploads, filled during Update and recorded into the frame command
    // buffer at BeginFrame. Each owns a host-visible staging buffer that is
    // deferred-deleted once the frame that copies from it has finished.
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
    // Buffer uploads are queued from worker threads (chunk meshes are built off the
    // main thread), so guard that queue. Image uploads stay main-thread only.
    std::mutex m_UploadMutex;

    // Deferred texture deletion, mirroring the buffer trash: image, view, and
    // descriptor set are freed together two frames after retirement.
    struct DeferredTexture {
        VkImage image;
        VmaAllocation allocation;
        VkImageView view;
        VkDescriptorSet set;
    };
    std::vector<DeferredTexture> m_PendingTextureTrash;
    std::array<std::vector<DeferredTexture>, k_MaxFramesInFlight> m_TextureTrashBins;

    // Per-frame-in-flight sync: the acquire semaphore and the fence guarding the
    // command buffer. The render-finished semaphore is per swapchain image so a
    // present never waits on a signal meant for a different image.
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    std::vector<VkFence> m_ImagesInFlight;

    uint32_t m_CurrentFrame = 0;
    uint32_t m_ImageIndex = 0;

    VkClearValue m_ClearColor = { { { 0.470f, 0.655f, 1.0f, 1.0f } } };
};

} // namespace Krafter
