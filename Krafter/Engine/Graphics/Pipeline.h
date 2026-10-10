#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "vulkan/vulkan.h"

namespace Krafter {

class Renderer;

struct PipelineConfig {
    std::string vertPath;
    std::string fragPath;

    uint32_t vertexStride = 0;
    uint32_t instanceStride = 0;
    std::vector<VkVertexInputAttributeDescription> attributes;

    uint32_t pushConstantSize = 0;

    bool useTextureSet = true;

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    bool depthTest = true;
    bool depthWrite = true;
    bool blend = true;

    VkBlendFactor srcColorFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dstColorFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    bool depthBias = false;
    float depthBiasConstant = 0.0f;
    float depthBiasSlope = 0.0f;
};

class Pipeline {
public:
    Pipeline(Renderer& renderer, const PipelineConfig& config);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    void Bind(VkCommandBuffer cmd) const;
    void PushConstants(VkCommandBuffer cmd, const void* data, uint32_t size) const;

    void BindTextureSet(VkCommandBuffer cmd, VkDescriptorSet set) const;

private:
    static VkShaderModule LoadModule(VkDevice device, const std::string& path);

    VkDevice m_Device = VK_NULL_HANDLE;
    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};

}
