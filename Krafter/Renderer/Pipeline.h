#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "vulkan/vulkan.h"

namespace Krafter {

// Everything that distinguishes one graphics pipeline from another. Vulkan bakes
// vertex layout, blend, cull, and depth state into the pipeline object rather than
// toggling them at draw time, so each distinct render pass state is one Pipeline.
struct PipelineConfig {
    std::string vertPath;
    std::string fragPath;

    // One interleaved vertex buffer at binding 0: its stride and the attributes
    // read from it. Empty for shaders that generate geometry without a vertex
    // buffer.
    uint32_t vertexStride = 0;
    std::vector<VkVertexInputAttributeDescription> attributes;

    // Size of the push-constant block, visible to both stages.
    uint32_t pushConstantSize = 0;

    // Whether the shader samples a texture (set 0, one combined image sampler).
    bool useTextureSet = true;

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    bool depthTest = true;
    bool depthWrite = true;
    bool blend = true;

    // Colour blend factors (used when blend is true). The default is standard alpha
    // blending; the UI's colour-inverting crosshair overrides them.
    VkBlendFactor srcColorFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dstColorFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    // Depth bias (the GL polygon offset): nudges fragments toward or away from the
    // camera so a coplanar decal (the crack overlay) sits on a face without
    // z-fighting. Negative factors pull toward the viewer.
    bool depthBias = false;
    float depthBiasConstant = 0.0f;
    float depthBiasSlope = 0.0f;
};

// A graphics pipeline plus its layout, built against the renderer's main render
// pass. Concrete renderers own one per distinct draw state.
class Pipeline {
public:
    explicit Pipeline(const PipelineConfig& config);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    void Bind(VkCommandBuffer cmd) const;
    void PushConstants(VkCommandBuffer cmd, const void* data, uint32_t size) const;

    // Binds `set` as descriptor set 0 (the sampled texture).
    void BindTextureSet(VkCommandBuffer cmd, VkDescriptorSet set) const;

private:
    static VkShaderModule LoadModule(VkDevice device, const std::string& path);

    VkPipelineLayout m_Layout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};

} // namespace Krafter
