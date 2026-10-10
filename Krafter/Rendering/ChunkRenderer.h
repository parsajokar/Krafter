#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#include "vulkan/vulkan.h"

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "Krafter/Engine/Graphics/Renderer.h"
#include "Krafter/Engine/JobSystem.h"
#include "Krafter/Engine/ResultQueue.h"
#include "Krafter/Rendering/ChunkMesh.h"
#include "Krafter/Rendering/WorldRenderer.h"

namespace Krafter {

class World;
class Sky;
struct RenderView;

class ChunkRenderer {
public:
    ChunkRenderer(Renderer& renderer, const World& world, JobSystem& jobs);

    ChunkRenderer(const ChunkRenderer&) = delete;
    ChunkRenderer& operator=(const ChunkRenderer&) = delete;

    void Update(const glm::vec3& cameraPosition, float deltaTime);

    void PrepareFrame(const RenderView& view);

    void RenderPass(ChunkPass pass, WorldRenderer& renderer, const RenderView& view, const Sky& sky) const;

    void RenderImGui();

private:
    struct MeshRecord {
        std::unique_ptr<ChunkMesh> mesh;

        uint64_t firstVersion = 0;
        uint64_t builtVersion = 0;
        uint64_t dispatchedVersion = 0;

        bool IsPending() const
        {
            return dispatchedVersion > builtVersion;
        }
    };

    struct MeshResult {
        glm::ivec2 position;
        ChunkMeshData data;
        uint64_t version;
    };

    struct DrawRun {
        uint32_t page;
        uint32_t first;
        uint32_t count;
    };

    struct VisibleChunk {
        const ChunkMesh* mesh;
        uint32_t instance;
        float distanceSq;
    };

    struct FrameBuffers {
        OwnedBuffer commands;
        void* commandsMapped = nullptr;
        size_t commandCapacity = 0;

        OwnedBuffer instances;
        void* instancesMapped = nullptr;
        size_t instanceCapacity = 0;
    };

    void DrainResults(float deltaTime);

    void EmitPass(ChunkPass pass);
    void UploadFrame();

    Renderer& m_Renderer;
    const World& m_World;
    JobSystem& m_JobSystem;

    std::unordered_map<glm::ivec2, MeshRecord> m_Meshes;
    ResultQueue<MeshResult> m_MeshResults;

    std::deque<MeshResult> m_NewMeshes;

    float m_UploadsPerSecond = 100.0f;
    float m_UploadCredit = 0.0f;

    std::vector<VisibleChunk> m_Visible;
    std::vector<VkDrawIndexedIndirectCommand> m_Commands;
    std::vector<WorldRenderer::ChunkInstance> m_Instances;
    std::array<std::vector<DrawRun>, k_ChunkPassCount> m_Runs;
    std::array<FrameBuffers, Renderer::k_MaxFramesInFlight> m_Frames;
    uint32_t m_FrameIndex = 0;

    bool m_FrustumCulling = true;
    size_t m_CulledCount = 0;
};

}
