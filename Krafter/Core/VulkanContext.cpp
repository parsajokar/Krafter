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

#include "Krafter/Core/VulkanContext.h"
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

void VkCheck(VkResult result, const char* what)
{
    if (result != VK_SUCCESS) {
        std::cerr << "[VULKAN] " << what << " failed (" << result << ")" << std::endl;
        std::abort();
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
    std::cerr << "[VULKAN] " << callbackData->pMessage << std::endl;
    assert(severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
    return VK_FALSE;
}

VulkanContext::VulkanContext(Window& window)
    : m_Window(window)
{
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateAllocator();
}

VulkanContext::~VulkanContext()
{
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

void VulkanContext::WaitIdle() const
{
    vkDeviceWaitIdle(m_Device);
}

void VulkanContext::CreateInstance()
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

    VkCheck(vkCreateInstance(&createInfo, nullptr, &m_Instance), "vkCreateInstance");
}

void VulkanContext::SetupDebugMessenger()
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
        VkCheck(create(m_Instance, &createInfo, nullptr, &m_DebugMessenger),
            "vkCreateDebugUtilsMessengerEXT");
    }
}

void VulkanContext::CreateSurface()
{
    VkCheck(glfwCreateWindowSurface(m_Instance, m_Window.GetId(), nullptr, &m_Surface),
        "glfwCreateWindowSurface");
}

void VulkanContext::PickPhysicalDevice()
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

void VulkanContext::CreateLogicalDevice()
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

    VkCheck(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), "vkCreateDevice");

    vkGetDeviceQueue(m_Device, m_GraphicsFamily, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_PresentFamily, 0, &m_PresentQueue);
}

void VulkanContext::CreateAllocator()
{
    VmaAllocatorCreateInfo createInfo = {};
    createInfo.instance = m_Instance;
    createInfo.physicalDevice = m_PhysicalDevice;
    createInfo.device = m_Device;
    createInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    VkCheck(vmaCreateAllocator(&createInfo, &m_Allocator), "vmaCreateAllocator");
}

VkFormat VulkanContext::FindDepthFormat() const
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

}
