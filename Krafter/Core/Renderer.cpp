#include "vulkan/vulkan.h"

#include "imgui.h"

#include "Krafter/Core/Renderer.h"

namespace Krafter {

void Renderer::SetClearColor(const glm::vec3& color)
{
    m_ClearColor.color = { { color.r, color.g, color.b, 1.0f } };
}

void Renderer::WaitIdle()
{
    m_Context.WaitIdle();
}

Renderer::Renderer(Window& window)
    : m_Context(window)
    , m_Swapchain(m_Context, window)
    , m_Buffers(m_Context, k_MaxFramesInFlight)
    , m_Textures(m_Context, k_MaxFramesInFlight)
{
    s_Instance = this;

    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();
}

Renderer::~Renderer()
{
    vkDeviceWaitIdle(m_Context.GetDevice());

    for (uint32_t i = 0; i < k_MaxFramesInFlight; i++) {
        vkDestroySemaphore(m_Context.GetDevice(), m_ImageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_Context.GetDevice(), m_InFlightFences[i], nullptr);
    }
    for (VkSemaphore semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Context.GetDevice(), semaphore, nullptr);
    }

    vkDestroyCommandPool(m_Context.GetDevice(), m_CommandPool, nullptr);
}

void Renderer::CreateCommandPool()
{
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = m_Context.GetGraphicsQueueFamily();
    VkCheck(vkCreateCommandPool(m_Context.GetDevice(), &createInfo, nullptr, &m_CommandPool), "vkCreateCommandPool");
}

void Renderer::CreateCommandBuffers()
{
    m_CommandBuffers.resize(k_MaxFramesInFlight);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = k_MaxFramesInFlight;
    VkCheck(vkAllocateCommandBuffers(m_Context.GetDevice(), &allocInfo, m_CommandBuffers.data()),
        "vkAllocateCommandBuffers");
}

void Renderer::CreateSyncObjects()
{
    m_ImageAvailableSemaphores.resize(k_MaxFramesInFlight);
    m_InFlightFences.resize(k_MaxFramesInFlight);
    m_RenderFinishedSemaphores.resize(m_Swapchain.GetImageCount());
    m_ImagesInFlight.resize(m_Swapchain.GetImageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < k_MaxFramesInFlight; i++) {
        VkCheck(vkCreateSemaphore(m_Context.GetDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]),
            "vkCreateSemaphore");
        VkCheck(vkCreateFence(m_Context.GetDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]), "vkCreateFence");
    }
    for (uint32_t i = 0; i < m_Swapchain.GetImageCount(); i++) {
        VkCheck(vkCreateSemaphore(m_Context.GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]),
            "vkCreateSemaphore");
    }
}

void Renderer::RecreateSwapchain()
{
    m_Swapchain.Recreate();

    for (VkSemaphore semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Context.GetDevice(), semaphore, nullptr);
    }
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    m_RenderFinishedSemaphores.resize(m_Swapchain.GetImageCount());
    for (uint32_t i = 0; i < m_Swapchain.GetImageCount(); i++) {
        VkCheck(vkCreateSemaphore(m_Context.GetDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]),
            "vkCreateSemaphore");
    }
    m_ImagesInFlight.assign(m_Swapchain.GetImageCount(), VK_NULL_HANDLE);
}

bool Renderer::BeginFrame()
{
    vkWaitForFences(m_Context.GetDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

    VkResult acquire = vkAcquireNextImageKHR(
        m_Context.GetDevice(), m_Swapchain.GetHandle(), UINT64_MAX,
        m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_ImageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return false;
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        VkCheck(acquire, "vkAcquireNextImageKHR");
    }

    if (m_ImagesInFlight[m_ImageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_Context.GetDevice(), 1, &m_ImagesInFlight[m_ImageIndex], VK_TRUE, UINT64_MAX);
    }
    m_ImagesInFlight[m_ImageIndex] = m_InFlightFences[m_CurrentFrame];

    vkResetFences(m_Context.GetDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

    m_Buffers.Recycle(m_CurrentFrame);
    m_Textures.Recycle(m_CurrentFrame);

    VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    VkCheck(vkBeginCommandBuffer(cmd, &beginInfo), "vkBeginCommandBuffer");

    m_Buffers.RecordUploads(cmd);

    VkClearValue clearValues[2];
    clearValues[0] = m_ClearColor;
    clearValues[1].depthStencil = { 1.0f, 0 };

    const VkExtent2D extent = m_Swapchain.GetExtent();

    VkRenderPassBeginInfo passInfo = {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passInfo.renderPass = m_Swapchain.GetRenderPass();
    passInfo.framebuffer = m_Swapchain.GetFramebuffer(m_ImageIndex);
    passInfo.renderArea.extent = extent;
    passInfo.clearValueCount = 2;
    passInfo.pClearValues = clearValues;
    vkCmdBeginRenderPass(cmd, &passInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(extent.height);
    viewport.width = static_cast<float>(extent.width);
    viewport.height = -static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    return true;
}

void Renderer::EndFrame()
{
    VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrame];
    vkCmdEndRenderPass(cmd);
    VkCheck(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");

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
    VkCheck(vkQueueSubmit(m_Context.GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]),
        "vkQueueSubmit");

    const VkSwapchainKHR swapchain = m_Swapchain.GetHandle();

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphores[m_ImageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &m_ImageIndex;

    VkResult present = vkQueuePresentKHR(m_Context.GetPresentQueue(), &presentInfo);
    if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain();
    } else if (present != VK_SUCCESS) {
        VkCheck(present, "vkQueuePresentKHR");
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % k_MaxFramesInFlight;
}

void Renderer::RenderImGui()
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_Context.GetPhysicalDevice(), &props);
    ImGui::Text("Vulkan Details:");
    ImGui::Text("Device: %s", props.deviceName);
    ImGui::Text("API: %u.%u.%u",
        VK_VERSION_MAJOR(props.apiVersion),
        VK_VERSION_MINOR(props.apiVersion),
        VK_VERSION_PATCH(props.apiVersion));
}

}
