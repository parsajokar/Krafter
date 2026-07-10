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

namespace Krafter {

class WorldRenderer;
class Sky;

class World {
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

    void SpawnDrop(const glm::vec3& position, Block block);

    std::vector<Block> TakeDrops()
    {
        return std::move(m_PendingDrops);
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
    };

    struct MeshResult {
        glm::ivec2 position;
        std::unique_ptr<ChunkMesh> mesh;
    };

    using LightResult = glm::ivec2;

    struct FallingBlock {
        glm::ivec3 cell;
        float delay;
    };

    static constexpr float k_FallStep = 0.05f;

    struct ItemDrop {
        glm::vec3 position;
        glm::vec3 velocity;
        Block block;
        float age;
        float phase;
    };

    static constexpr float k_DropGravity = 24.0f;
    static constexpr float k_DropPopUp = 3.0f;
    static constexpr float k_DropPopOut = 1.2f;
    static constexpr float k_DropHalfHeight = 0.125f;
    static constexpr float k_PickupDelay = 0.5f;
    static constexpr float k_AttractRadius = 4.0f;
    static constexpr float k_PickupRadius = 0.75f;
    static constexpr float k_AttractMinSpeed = 4.0f;
    static constexpr float k_AttractMaxSpeed = 16.0f;
    static constexpr float k_DropLifetime = 300.0f;
    static constexpr float k_PlayerEyeHeight = 1.62f;
    static constexpr float k_DropVoidY = -16.0f;
    static constexpr float k_DropBobSpeed = 3.0f;
    static constexpr float k_DropBobAmplitude = 0.1f;
    static constexpr float k_DropSize = 0.4f;

    static constexpr bool IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius);

    std::array<std::shared_ptr<Chunk>, 9> Gather3x3(const glm::ivec2& center) const;
    static std::array<const Chunk*, 9> AsPointers(const std::array<std::shared_ptr<Chunk>, 9>& grid);

    void DrainResults();

    bool CanEdit(const glm::ivec2& chunkPosition) const;

    void BreakCactusColumn(const glm::ivec3& worldPosition);

    void ChopFloatingTree(const glm::ivec3& brokenPosition);

    void ScheduleFall(const std::vector<glm::ivec3>& cells, const glm::ivec3& origin);

    void UpdateFallingBlocks(float deltaTime);

    void UpdateDrops(float deltaTime, const glm::vec3& cameraPosition);

    void InvalidateChunk(const glm::ivec2& chunkPosition);

    bool HasAllTerrainNeighbours(const glm::ivec2& chunkPosition) const;
    bool HasAllLitNeighbours(const glm::ivec2& chunkPosition) const;

    std::unordered_map<glm::ivec2, ChunkRecord> m_Chunks;
    int32_t m_RenderDistance = 10;
    int32_t m_MaxMeshUploadsPerFrame = 8;

    std::unordered_set<glm::ivec2> m_PendingTerrain;
    std::unordered_set<glm::ivec2> m_PendingLight;
    std::unordered_set<glm::ivec2> m_PendingMesh;

    std::vector<FallingBlock> m_FallingBlocks;
    std::unordered_set<glm::ivec3> m_Falling;

    std::vector<ItemDrop> m_Drops;
    std::vector<Block> m_PendingDrops;
    float m_Time = 0.0f;
    glm::vec3 m_LastCameraPosition = glm::vec3(0.0f);

    ResultQueue<TerrainResult> m_TerrainResults;
    ResultQueue<LightResult> m_LightResults;
    ResultQueue<MeshResult> m_MeshResults;

    // Declared last so it is destroyed first: its workers must stop before the
    // chunk/result state they read is torn down. Do not move this up.
    JobSystem m_JobSystem;
};

}
