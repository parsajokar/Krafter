#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <vector>

#include "vulkan/vulkan.h"

#include "vk_mem_alloc.h"

#include "GLFW/glfw3.h"

#include "imgui.h"

#include "Krafter/Core/Renderer.h"
#include "Krafter/Core/Window.h"

namespace Krafter {

namespace {

#ifdef NDEBUG
constexpr bool k_EnableValidation = false;
#else
constexpr bool k_EnableValidation = true;
#endif

const std::array<const char*, 1> k_ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::array<const char*, 1> k_DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

void Check(VkResult result, const char* what)
{
    if (result != VK_SUCCESS) {
        std::cerr << "[VULKAN] " << what << " failed (" << result << ")" << std::endl;
        std::abort();
    }
}

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

bool ValidationLayersSupported()
{
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> available(count);
    vkEnumerateInstanceLayerProperties(&count, available.data());

    for (const char* wanted : k_ValidationLayers) {
        bool found = false;
        for (const VkLayerProperties& layer : available) {
            if (std::strcmp(wanted, layer.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    std::cerr << "[VULKAN] " << callbackData->pMessage << std::endl;
    assert(severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
    return VK_FALSE;
}

void Renderer::SetClearColor(const glm::vec3& color)
{
    m_ClearColor.color = { { color.r, color.g, color.b, 1.0f } };
}

void Renderer::WaitIdle()
{
    vkDeviceWaitIdle(m_Device);
}

Renderer::Renderer(Window& window)
    : m_Window(window)
{
    s_Instance = this;

    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateAllocator();
    CreateSwapchain();
    CreateImageViews();
    CreateDepthResources();
    CreateRenderPass();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();
    CreateTextureInfrastructure();
}

Renderer::~Renderer()
{
    vkDeviceWaitIdle(m_Device);

    for (auto& bin : m_TrashBins) {
        for (const GpuBuffer& buffer : bin) {
            vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);
        }
    }
    for (const GpuBuffer& buffer : m_PendingTrash) {
        vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);
    }
    for (const GpuBuffer& buffer : m_ReadyToFree) {
        vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);
    }
    for (const PendingBufferUpload& upload : m_PendingBufferUploads) {
        vmaDestroyBuffer(m_Allocator, upload.staging.buffer, upload.staging.allocation);
    }
    for (const PendingImageUpload& upload : m_PendingImageUploads) {
        vmaDestroyBuffer(m_Allocator, upload.staging.buffer, upload.staging.allocation);
    }
    for (auto& bin : m_TextureTrashBins) {
        for (const DeferredTexture& texture : bin) {
            vkFreeDescriptorSets(m_Device, m_TexturePool, 1, &texture.set);
            vkDestroyImageView(m_Device, texture.view, nullptr);
            vmaDestroyImage(m_Allocator, texture.image, texture.allocation);
        }
    }
    for (const DeferredTexture& texture : m_PendingTextureTrash) {
        vkFreeDescriptorSets(m_Device, m_TexturePool, 1, &texture.set);
        vkDestroyImageView(m_Device, texture.view, nullptr);
        vmaDestroyImage(m_Allocator, texture.image, texture.allocation);
    }

    for (uint32_t i = 0; i < k_MaxFramesInFlight; i++) {
        vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
    }
    for (VkSemaphore semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Device, semaphore, nullptr);
    }

    vkDestroySampler(m_Device, m_NearestSampler, nullptr);
    vkDestroyDescriptorPool(m_Device, m_TexturePool, nullptr);
    vkDestroyDescriptorSetLayout(m_Device, m_TextureSetLayout, nullptr);

    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

    CleanupSwapchain();
    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

    vmaDestroyAllocator(m_Allocator);

    vkDestroyDevice(m_Device, nullptr);

    if (m_DebugMessenger != VK_NULL_HANDLE) {
        auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroy) {
            destroy(m_Instance, m_DebugMessenger, nullptr);
        }
    }

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    vkDestroyInstance(m_Instance, nullptr);
}

void Renderer::CreateInstance()
{
    if (k_EnableValidation && !ValidationLayersSupported()) {
        std::cerr << "[VULKAN] Validation layers requested but not available" << std::endl;
        std::abort();
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Krafter";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Krafter";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (k_EnableValidation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    if (k_EnableValidation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(k_ValidationLayers.size());
        createInfo.ppEnabledLayerNames = k_ValidationLayers.data();
    }

    Check(vkCreateInstance(&createInfo, nullptr, &m_Instance), "vkCreateInstance");
}

void Renderer::SetupDebugMessenger()
{
    if (!k_EnableValidation) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;

    auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
    if (create) {
        Check(create(m_Instance, &createInfo, nullptr, &m_DebugMessenger),
            "vkCreateDebugUtilsMessengerEXT");
    }
}

void Renderer::CreateSurface()
{
    Check(glfwCreateWindowSurface(m_Instance, m_Window.GetId(), nullptr, &m_Surface),
        "glfwCreateWindowSurface");
}

void Renderer::PickPhysicalDevice()
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
    if (count == 0) {
        std::cerr << "[VULKAN] No Vulkan-capable GPU found" << std::endl;
        std::abort();
    }
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

    VkPhysicalDevice fallback = VK_NULL_HANDLE;
    for (VkPhysicalDevice device : devices) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

        bool hasGraphics = false;
        bool hasPresent = false;
        uint32_t graphics = 0;
        uint32_t present = 0;
        for (uint32_t i = 0; i < familyCount; i++) {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                hasGraphics = true;
                graphics = i;
            }
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
            if (presentSupport) {
                hasPresent = true;
                present = i;
            }
        }
        if (!hasGraphics || !hasPresent) {
            continue;
        }

        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> exts(extCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, exts.data());
        bool hasSwapchain = false;
        for (const VkExtensionProperties& ext : exts) {
            if (std::strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                hasSwapchain = true;
                break;
            }
        }
        if (!hasSwapchain) {
            continue;
        }

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            m_PhysicalDevice = device;
            m_GraphicsFamily = graphics;
            m_PresentFamily = present;
            break;
        }
        if (fallback == VK_NULL_HANDLE) {
            fallback = device;
            m_GraphicsFamily = graphics;
            m_PresentFamily = present;
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE) {
        if (fallback == VK_NULL_HANDLE) {
            std::cerr << "[VULKAN] No suitable GPU found" << std::endl;
            std::abort();
        }
        m_PhysicalDevice = fallback;
    }

    VkPhysicalDeviceProperties chosen;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &chosen);
    const char* type = chosen.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        ? "discrete"
        : "integrated/other";
    std::cerr << "[VULKAN] Using GPU: " << chosen.deviceName << " (" << type << ")" << std::endl;
}

void Renderer::CreateLogicalDevice()
{
    std::set<uint32_t> uniqueFamilies = { m_GraphicsFamily, m_PresentFamily };
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    const float priority = 1.0f;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo queueInfo = {};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        queueInfos.push_back(queueInfo);
    }

    VkPhysicalDeviceFeatures features = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.pEnabledFeatures = &features;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(k_DeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = k_DeviceExtensions.data();

    Check(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), "vkCreateDevice");

    vkGetDeviceQueue(m_Device, m_GraphicsFamily, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_PresentFamily, 0, &m_PresentQueue);
}

void Renderer::CreateAllocator()
{
    VmaAllocatorCreateInfo createInfo = {};
    createInfo.instance = m_Instance;
    createInfo.physicalDevice = m_PhysicalDevice;
    createInfo.device = m_Device;
    createInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    Check(vmaCreateAllocator(&createInfo, &m_Allocator), "vmaCreateAllocator");
}

VkFormat Renderer::FindDepthFormat() const
{
    const VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
    };
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }
    std::cerr << "[VULKAN] No supported depth format" << std::endl;
    std::abort();
}

void Renderer::CreateDepthResources()
{
    m_DepthFormat = FindDepthFormat();

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = m_DepthFormat;
    imageInfo.extent = { m_SwapchainExtent.width, m_SwapchainExtent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    Check(vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &m_DepthImage, &m_DepthAllocation, nullptr),
        "vmaCreateImage (depth)");

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_DepthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_DepthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    Check(vkCreateImageView(m_Device, &viewInfo, nullptr, &m_DepthView), "vkCreateImageView (depth)");
}

void Renderer::CreateSwapchain()
{
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &caps);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, formats.data());
    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const VkSurfaceFormatKHR& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = format;
            break;
        }
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, presentModes.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    bool hasMailbox = false;
    bool hasImmediate = false;
    for (VkPresentModeKHR mode : presentModes) {
        hasMailbox = hasMailbox || mode == VK_PRESENT_MODE_MAILBOX_KHR;
        hasImmediate = hasImmediate || mode == VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    if (hasMailbox) {
        presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    } else if (hasImmediate) {
        presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    VkExtent2D extent;
    if (caps.currentExtent.width != UINT32_MAX) {
        extent = caps.currentExtent;
    } else {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_Window.GetId(), &width, &height);
        extent.width = std::clamp(static_cast<uint32_t>(width),
            caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(height),
            caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    uint32_t imageCount = caps.minImageCount + 1;
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR && imageCount < 3) {
        imageCount = 3;
    }
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    const uint32_t families[] = { m_GraphicsFamily, m_PresentFamily };
    if (m_GraphicsFamily != m_PresentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = families;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    Check(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain), "vkCreateSwapchainKHR");

    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
    m_SwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());

    m_SwapchainFormat = surfaceFormat.format;
    m_SwapchainExtent = extent;
}

void Renderer::CreateImageViews()
{
    m_SwapchainImageViews.resize(m_SwapchainImages.size());
    for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_SwapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_SwapchainFormat;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.layerCount = 1;
        Check(vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapchainImageViews[i]),
            "vkCreateImageView");
    }
}

void Renderer::CreateRenderPass()
{
    VkAttachmentDescription color = {};
    color.format = m_SwapchainFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth = {};
    depth.format = m_DepthFormat;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef = {};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef = {};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    const VkAttachmentDescription attachments[] = { color, depth };

    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 2;
    createInfo.pAttachments = attachments;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    Check(vkCreateRenderPass(m_Device, &createInfo, nullptr, &m_RenderPass), "vkCreateRenderPass");
}

void Renderer::CreateFramebuffers()
{
    m_Framebuffers.resize(m_SwapchainImageViews.size());
    for (size_t i = 0; i < m_SwapchainImageViews.size(); i++) {
        VkImageView attachments[] = { m_SwapchainImageViews[i], m_DepthView };

        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = m_RenderPass;
        createInfo.attachmentCount = 2;
        createInfo.pAttachments = attachments;
        createInfo.width = m_SwapchainExtent.width;
        createInfo.height = m_SwapchainExtent.height;
        createInfo.layers = 1;
        Check(vkCreateFramebuffer(m_Device, &createInfo, nullptr, &m_Framebuffers[i]),
            "vkCreateFramebuffer");
    }
}

void Renderer::CreateCommandPool()
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = m_GraphicsFamily;
    Check(vkCreateCommandPool(m_Device, &createInfo, nullptr, &m_CommandPool), "vkCreateCommandPool");
}

void Renderer::CreateCommandBuffers()
{
    m_CommandBuffers.resize(k_MaxFramesInFlight);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = k_MaxFramesInFlight;
    Check(vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()),
        "vkAllocateCommandBuffers");
}

void Renderer::CreateSyncObjects()
{
    m_ImageAvailableSemaphores.resize(k_MaxFramesInFlight);
    m_InFlightFences.resize(k_MaxFramesInFlight);
    m_RenderFinishedSemaphores.resize(m_SwapchainImages.size());
    m_ImagesInFlight.resize(m_SwapchainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < k_MaxFramesInFlight; i++) {
        Check(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]),
            "vkCreateSemaphore");
        Check(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]), "vkCreateFence");
    }
    for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
        Check(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]),
            "vkCreateSemaphore");
    }
}

void Renderer::CreateTextureInfrastructure()
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    Check(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_NearestSampler), "vkCreateSampler");

    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    Check(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_TextureSetLayout),
        "vkCreateDescriptorSetLayout");

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 64;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 64;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    Check(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_TexturePool), "vkCreateDescriptorPool");
}

GpuBuffer Renderer::CreateDeviceLocalBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage)
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
    Check(vmaCreateBuffer(m_Allocator, &stagingInfo, &stagingAlloc, &staging.buffer, &staging.allocation, &stagingInfoOut),
        "vmaCreateBuffer (staging)");
    std::memcpy(stagingInfoOut.pMappedData, data, static_cast<size_t>(size));
    vmaFlushAllocation(m_Allocator, staging.allocation, 0, size);

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo deviceAlloc = {};
    deviceAlloc.usage = VMA_MEMORY_USAGE_AUTO;

    GpuBuffer result;
    Check(vmaCreateBuffer(m_Allocator, &bufferInfo, &deviceAlloc, &result.buffer, &result.allocation, nullptr),
        "vmaCreateBuffer (device)");

    {
        std::lock_guard<std::mutex> lock(m_UploadMutex);
        m_PendingBufferUploads.push_back({ staging, result.buffer, size });
    }
    return result;
}

GpuBuffer Renderer::CreateHostVisibleBuffer(VkDeviceSize size, VkBufferUsageFlags usage, void** mappedOut)
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
    Check(vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, &info),
        "vmaCreateBuffer (host visible)");
    *mappedOut = info.pMappedData;
    return buffer;
}

void Renderer::QueueImageUpload(VkImage image, VkImageLayout oldLayout,
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
    Check(vmaCreateBuffer(m_Allocator, &stagingInfo, &stagingAlloc, &staging.buffer, &staging.allocation, &stagingInfoOut),
        "vmaCreateBuffer (image staging)");
    std::memcpy(stagingInfoOut.pMappedData, pixels, static_cast<size_t>(size));
    vmaFlushAllocation(m_Allocator, staging.allocation, 0, size);

    m_PendingImageUploads.push_back({ staging, image, oldLayout, x, y, width, height });
}

void Renderer::RecordPendingUploads(VkCommandBuffer cmd)
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

void Renderer::DestroyBuffer(const GpuBuffer& buffer)
{
    if (buffer.buffer != VK_NULL_HANDLE) {
        m_PendingTrash.push_back(buffer);
    }
}

void Renderer::FlushHostBuffer(const GpuBuffer& buffer, VkDeviceSize offset, VkDeviceSize size)
{
    vmaFlushAllocation(m_Allocator, buffer.allocation, offset, size);
}

VkDescriptorSet Renderer::AllocateTextureSet(VkImageView view)
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_TexturePool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_TextureSetLayout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    Check(vkAllocateDescriptorSets(m_Device, &allocInfo, &set), "vkAllocateDescriptorSets");

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.sampler = m_NearestSampler;
    imageInfo.imageView = view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);

    return set;
}

void Renderer::DestroyTexture(VkImage image, VmaAllocation allocation, VkImageView view, VkDescriptorSet set)
{
    m_PendingTextureTrash.push_back({ image, allocation, view, set });
}

VkCommandBuffer Renderer::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(m_Device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    return cmd;
}

void Renderer::EndSingleTimeCommands(VkCommandBuffer cmd)
{
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue);

    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &cmd);
}

void Renderer::CleanupSwapchain()
{
    vkDestroyImageView(m_Device, m_DepthView, nullptr);
    vmaDestroyImage(m_Allocator, m_DepthImage, m_DepthAllocation);
    m_DepthView = VK_NULL_HANDLE;
    m_DepthImage = VK_NULL_HANDLE;

    for (VkFramebuffer framebuffer : m_Framebuffers) {
        vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
    }
    m_Framebuffers.clear();

    for (VkImageView view : m_SwapchainImageViews) {
        vkDestroyImageView(m_Device, view, nullptr);
    }
    m_SwapchainImageViews.clear();

    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
    m_Swapchain = VK_NULL_HANDLE;
}

void Renderer::RecreateSwapchain()
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_Window.GetId(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window.GetId(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_Device);

    CleanupSwapchain();

    CreateSwapchain();
    CreateImageViews();
    CreateDepthResources();
    CreateFramebuffers();

    for (VkSemaphore semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Device, semaphore, nullptr);
    }
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_RenderFinishedSemaphores.resize(m_SwapchainImages.size());
    for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
        Check(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]),
            "vkCreateSemaphore");
    }
    m_ImagesInFlight.assign(m_SwapchainImages.size(), VK_NULL_HANDLE);
}

bool Renderer::BeginFrame()
{
    vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

    VkResult acquire = vkAcquireNextImageKHR(
        m_Device, m_Swapchain, UINT64_MAX,
        m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_ImageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return false;
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        Check(acquire, "vkAcquireNextImageKHR");
    }

    if (m_ImagesInFlight[m_ImageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_Device, 1, &m_ImagesInFlight[m_ImageIndex], VK_TRUE, UINT64_MAX);
    }
    m_ImagesInFlight[m_ImageIndex] = m_InFlightFences[m_CurrentFrame];

    vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

    for (const GpuBuffer& buffer : m_TrashBins[m_CurrentFrame]) {
        m_ReadyToFree.push_back(buffer);
    }
    m_TrashBins[m_CurrentFrame].clear();
    m_TrashBins[m_CurrentFrame].swap(m_PendingTrash);

    const size_t budget = std::max<size_t>(k_MaxFreesPerFrame, m_ReadyToFree.size() / 4);
    for (size_t freed = 0; freed < budget && !m_ReadyToFree.empty(); freed++) {
        const GpuBuffer& buffer = m_ReadyToFree.front();
        vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);
        m_ReadyToFree.pop_front();
    }

    for (const DeferredTexture& texture : m_TextureTrashBins[m_CurrentFrame]) {
        vkFreeDescriptorSets(m_Device, m_TexturePool, 1, &texture.set);
        vkDestroyImageView(m_Device, texture.view, nullptr);
        vmaDestroyImage(m_Allocator, texture.image, texture.allocation);
    }
    m_TextureTrashBins[m_CurrentFrame].clear();
    m_TextureTrashBins[m_CurrentFrame].swap(m_PendingTextureTrash);

    VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    Check(vkBeginCommandBuffer(cmd, &beginInfo), "vkBeginCommandBuffer");

    RecordPendingUploads(cmd);

    VkClearValue clearValues[2];
    clearValues[0] = m_ClearColor;
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo passInfo = {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass = m_RenderPass;
    passInfo.framebuffer = m_Framebuffers[m_ImageIndex];
    passInfo.renderArea.extent = m_SwapchainExtent;
    passInfo.clearValueCount = 2;
    passInfo.pClearValues = clearValues;
    vkCmdBeginRenderPass(cmd, &passInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(m_SwapchainExtent.height);
    viewport.width = static_cast<float>(m_SwapchainExtent.width);
    viewport.height = -static_cast<float>(m_SwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.extent = m_SwapchainExtent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    return true;
}

void Renderer::EndFrame()
{
    VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrame];
    vkCmdEndRenderPass(cmd);
    Check(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrame];
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphores[m_ImageIndex];
    Check(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]),
        "vkQueueSubmit");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_ImageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain;
    presentInfo.pImageIndices = &m_ImageIndex;

    VkResult present = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
    if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain();
    } else if (present != VK_SUCCESS) {
        Check(present, "vkQueuePresentKHR");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % k_MaxFramesInFlight;
}

void Renderer::RenderImGui()
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
    ImGui::Text("Vulkan Details:");
    ImGui::Text("Device: %s", props.deviceName);
    ImGui::Text("API: %u.%u.%u",
        VK_VERSION_MAJOR(props.apiVersion),
        VK_VERSION_MINOR(props.apiVersion),
        VK_VERSION_PATCH(props.apiVersion));
}

}
