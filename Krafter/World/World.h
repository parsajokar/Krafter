#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "Krafter/Core/JobSystem.h"
#include "Krafter/Core/ResultQueue.h"
#include "Krafter/Renderer/ChunkMesh.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Chunk.h"
#include "Krafter/World/DropSystem.h"
#include "Krafter/World/FallingBlockSystem.h"
#include "Krafter/World/TerrainGenerator.h"

namespace Krafter {

class WorldRenderer;
class Sky;

class World {
    friend class FallingBlockSystem;

public:
    World(int32_t seed);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void Update(const glm::vec3& cameraPosition, float deltaTime);
    void Render(WorldRenderer& renderer, const glm::mat4& viewProjection, const Sky& sky);
    void RenderImGui();

    Block GetBlock(const glm::ivec3& worldPosition) const;
    void SetBlock(const glm::ivec3& worldPosition, Block block);

    bool IsChunkLoaded(const glm::vec3& worldPosition) const;

    void PlaceBlock(const glm::ivec3& worldPosition, Block block);

    bool RaycastBlock(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, glm::ivec3& outHit, glm::ivec3& outBefore) const;

    void SpawnDrop(const glm::vec3& position, Block block)
    {
        m_DropSystem.Spawn(position, block);
    }

    std::vector<Block> TakeDrops()
    {
        return m_DropSystem.TakePickedUp();
    }

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
        uint32_t generation;
    };

    struct MeshResult {
        glm::ivec2 position;
        std::unique_ptr<ChunkMesh> mesh;
        uint32_t generation;
    };

    struct LightResult {
        glm::ivec2 position;
        uint32_t generation;
    };

    static constexpr bool IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius);

    std::array<std::shared_ptr<Chunk>, 9> Gather3x3(const glm::ivec2& center) const;
    static std::array<const Chunk*, 9> AsPointers(const std::array<std::shared_ptr<Chunk>, 9>& grid);

    void DrainResults(float deltaTime);

    void Reload();

    bool CanEdit(const glm::ivec2& chunkPosition) const;

    void BreakCactusColumn(const glm::ivec3& worldPosition);

    void ChopFloatingTree(const glm::ivec3& brokenPosition);

    void InvalidateChunk(const glm::ivec2& chunkPosition);

    bool HasAllTerrainNeighbours(const glm::ivec2& chunkPosition) const;
    bool HasAllLitNeighbours(const glm::ivec2& chunkPosition) const;

    std::unordered_map<glm::ivec2, ChunkRecord> m_Chunks;
    int32_t m_RenderDistance = 10;

    float m_ChunkUploadsPerSecond = 100.0f;
    float m_ChunkRemovalsPerSecond = 100.0f;
    float m_UploadCredit = 0.0f;
    float m_RemovalCredit = 0.0f;

    std::unordered_set<glm::ivec2> m_PendingTerrain;
    std::unordered_set<glm::ivec2> m_PendingLight;
    std::unordered_set<glm::ivec2> m_PendingMesh;

    uint32_t m_Generation = 0;

    TerrainGenerator m_Generator;
    FallingBlockSystem m_FallingSystem;
    DropSystem m_DropSystem;

    ResultQueue<TerrainResult> m_TerrainResults;
    ResultQueue<LightResult> m_LightResults;
    ResultQueue<MeshResult> m_MeshResults;

    // Declared last so it is destroyed first: its workers must stop before the
    // chunk/result state they read is torn down. Do not move this up.
    JobSystem m_JobSystem;
};

}
