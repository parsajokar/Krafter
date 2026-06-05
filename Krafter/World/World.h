#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Chunk.h"

namespace Krafter {

class Renderer;
class Sky;

class World {
public:
    World();
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void Update(const glm::vec3& cameraPosition);
    void Render(Renderer& renderer, const glm::mat4& viewProjection, const Sky& sky);
    void RenderImGui();

    Block GetBlock(const glm::ivec3& worldPosition) const;

private:
    enum class ChunkState {
        k_TerrainReady,
        k_LightReady,
        k_MeshReady
    };

    struct ChunkRecord {
        std::shared_ptr<Chunk> chunk;
        std::unique_ptr<ChunkMesh> mesh;
        ChunkState state;

        ChunkRecord(std::shared_ptr<Chunk> chunk, ChunkState state)
            : chunk(std::move(chunk))
            , state(state)
        {
        }
    };

    struct TerrainResult {
        glm::ivec2 position;
        std::shared_ptr<Chunk> chunk;
    };

    struct MeshResult {
        glm::ivec2 position;
        ChunkMeshData data;
    };

    // The lighting pass writes directly into the chunk's storage, so a finished
    // job only needs to report which chunk became lit.
    using LightResult = glm::ivec2;

    static constexpr bool IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius);

    void WorkerLoop();
    void DispatchJob(std::function<void()> job);

    void DrainResults();

    // Both stages need the full 3x3 neighbourhood: lighting for block context,
    // meshing so border smooth-lighting and AO read settled neighbour light.
    bool HasAllTerrainNeighbours(const glm::ivec2& chunkPosition) const;
    bool HasAllLitNeighbours(const glm::ivec2& chunkPosition) const;

    std::unordered_map<glm::ivec2, ChunkRecord> m_Chunks;
    int32_t m_RenderDistance = 10;
    int32_t m_MaxMeshUploadsPerFrame = 8;

    std::unordered_set<glm::ivec2> m_PendingTerrain;
    std::unordered_set<glm::ivec2> m_PendingLight;
    std::unordered_set<glm::ivec2> m_PendingMesh;

    std::mutex m_JobMutex;
    std::condition_variable m_JobCv;
    std::deque<std::function<void()>> m_Jobs;

    std::mutex m_TerrainResultMutex;
    std::deque<TerrainResult> m_TerrainResults;

    std::mutex m_LightResultMutex;
    std::deque<LightResult> m_LightResults;

    std::mutex m_MeshResultMutex;
    std::deque<MeshResult> m_MeshResults;

    std::atomic<bool> m_Stop = false;
    std::vector<std::thread> m_Workers;
};

} // namespace Krafter
