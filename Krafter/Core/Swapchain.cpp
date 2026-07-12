#include <algorithm>
#include <vector>

#include "vulkan/vulkan.h"

#include "vk_mem_alloc.h"

#include "GLFW/glfw3.h"

#include "Krafter/Core/Swapchain.h"
#include "Krafter/Core/VulkanContext.h"
#include "Krafter/Core/Window.h"

namespace Krafter {

Swapchain::Swapchain(VulkanContext& context, Window& window)
    : m_Context(context)
    , m_Window(window)
{
    CreateSwapchain();
    CreateImageViews();
    CreateDepthResources();
    CreateRenderPass();
    CreateFramebuffers();
}

Swapchain::~Swapchain()
{
    Cleanup();
    vkDestroyRenderPass(m_Context.GetDevice(), m_RenderPass, nullptr);
}

void Swapchain::Recreate()
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_Window.GetId(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window.GetId(), &width, &height);
        glfwWaitEvents();
    }

    m_Context.WaitIdle();

    Cleanup();

    CreateSwapchain();
    CreateImageViews();
    CreateDepthResources();
    CreateFramebuffers();
}

void Swapchain::CreateSwapchain()
{
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Context.GetPhysicalDevice(), m_Context.GetSurface(), &caps);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context.GetPhysicalDevice(), m_Context.GetSurface(), &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Context.GetPhysicalDevice(), m_Context.GetSurface(), &formatCount, formats.data());
    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const VkSurfaceFormatKHR& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = format;
            break;
        }
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Context.GetPhysicalDevice(), m_Context.GetSurface(), &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Context.GetPhysicalDevice(), m_Context.GetSurface(), &presentModeCount, presentModes.data());

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
    createInfo.surface = m_Context.GetSurface();
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

    const uint32_t families[] = { m_Context.GetGraphicsQueueFamily(), m_Context.GetPresentQueueFamily() };
    if (m_Context.GetGraphicsQueueFamily() != m_Context.GetPresentQueueFamily()) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = families;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkCheck(vkCreateSwapchainKHR(m_Context.GetDevice(), &createInfo, nullptr, &m_Swapchain), "vkCreateSwapchainKHR");

    vkGetSwapchainImagesKHR(m_Context.GetDevice(), m_Swapchain, &imageCount, nullptr);
    m_Images.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Context.GetDevice(), m_Swapchain, &imageCount, m_Images.data());

    m_Format = surfaceFormat.format;
    m_Extent = extent;
}

void Swapchain::CreateImageViews()
{
    m_ImageViews.resize(m_Images.size());
    for (size_t i = 0; i < m_Images.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_Images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_Format;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.layerCount = 1;
        VkCheck(vkCreateImageView(m_Context.GetDevice(), &createInfo, nullptr, &m_ImageViews[i]),
            "vkCreateImageView");
    }
}

void Swapchain::CreateDepthResources()
{
    m_DepthFormat = m_Context.FindDepthFormat();

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = m_DepthFormat;
    imageInfo.extent = { m_Extent.width, m_Extent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    VkCheck(vmaCreateImage(m_Context.GetAllocator(), &imageInfo, &allocInfo, &m_DepthImage, &m_DepthAllocation, nullptr),
        "vmaCreateImage (depth)");

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_DepthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_DepthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    VkCheck(vkCreateImageView(m_Context.GetDevice(), &viewInfo, nullptr, &m_DepthView), "vkCreateImageView (depth)");
}

void Swapchain::CreateRenderPass()
{
    VkAttachmentDescription color = {};
    color.format = m_Format;
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

    VkCheck(vkCreateRenderPass(m_Context.GetDevice(), &createInfo, nullptr, &m_RenderPass), "vkCreateRenderPass");
}

void Swapchain::CreateFramebuffers()
{
    m_Framebuffers.resize(m_ImageViews.size());
    for (size_t i = 0; i < m_ImageViews.size(); i++) {
        VkImageView attachments[] = { m_ImageViews[i], m_DepthView };

        VkFramebufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = m_RenderPass;
        createInfo.attachmentCount = 2;
        createInfo.pAttachments = attachments;
        createInfo.width = m_Extent.width;
        createInfo.height = m_Extent.height;
        createInfo.layers = 1;
        VkCheck(vkCreateFramebuffer(m_Context.GetDevice(), &createInfo, nullptr, &m_Framebuffers[i]),
            "vkCreateFramebuffer");
    }
}

void Swapchain::Cleanup()
{
    vkDestroyImageView(m_Context.GetDevice(), m_DepthView, nullptr);
    vmaDestroyImage(m_Context.GetAllocator(), m_DepthImage, m_DepthAllocation);
    m_DepthView = VK_NULL_HANDLE;
    m_DepthImage = VK_NULL_HANDLE;

    for (VkFramebuffer framebuffer : m_Framebuffers) {
        vkDestroyFramebuffer(m_Context.GetDevice(), framebuffer, nullptr);
    }
    m_Framebuffers.clear();

    for (VkImageView view : m_ImageViews) {
        vkDestroyImageView(m_Context.GetDevice(), view, nullptr);
    }
    m_ImageViews.clear();

    vkDestroySwapchainKHR(m_Context.GetDevice(), m_Swapchain, nullptr);
    m_Swapchain = VK_NULL_HANDLE;
}

}
