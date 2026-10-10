#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "Krafter/Engine/JobSystem.h"
#include "Krafter/Engine/ResultQueue.h"
#include "Krafter/World/Block.h"
#include "Krafter/World/Chunk.h"
#include "Krafter/World/Coords.h"
#include "Krafter/World/DropSystem.h"
#include "Krafter/World/FallingBlockSystem.h"
#include "Krafter/World/FluidSystem.h"
#include "Krafter/World/TerrainGenerator.h"
#include "Krafter/World/WorldAccess.h"

namespace Krafter {

class World final : public WorldAccess {
public:
    World(int32_t seed, JobSystem& jobs);
    ~World() override;

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    void Tick(float step, const glm::vec3& playerPosition, ItemCollector& collector);

    void Update(const glm::vec3& cameraPosition, float deltaTime);

    void RenderImGui();

    Block GetBlock(const glm::ivec3& worldPosition) const override;
    uint8_t GetFluid(const glm::ivec3& worldPosition) const override;

    bool SetBlock(const glm::ivec3& worldPosition, Block block) override;
    bool SetFluid(const glm::ivec3& worldPosition, Block fluid, uint8_t level) override;

    bool IsChunkLoaded(const glm::vec3& worldPosition) const;

    bool PlaceBlock(const glm::ivec3& worldPosition, Block block);

    bool RaycastBlock(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, glm::ivec3& outHit, glm::ivec3& outBefore) const;

    void SpawnDrop(const glm::vec3& position, const Item& item) override
    {
        m_DropSystem.Spawn(position, item);
    }

    const DropSystem& GetDrops() const
    {
        return m_DropSystem;
    }

    bool IsLoadedAround(const glm::ivec3& cell) const override;
    bool IsLoadedOrKept(const glm::ivec3& cell) const override;

    void ScheduleFall(const std::vector<glm::ivec3>& cells, const glm::ivec3& origin) override
    {
        m_FallingSystem.Schedule(cells, origin);
    }

    bool IsFalling(const glm::ivec3& cell) const override
    {
        return m_FallingSystem.IsFalling(cell);
    }

    void ScheduleFluid(const glm::ivec3& cell, Block fluid) override
    {
        m_FluidSystem.Schedule(cell, fluid);
    }

    const Climate& GetClimate() const
    {
        return m_Generator.GetClimate();
    }

    int32_t GetRenderDistance() const
    {
        return m_RenderDistance;
    }

    bool HasChunk(const glm::ivec2& chunkPosition) const
    {
        return m_Chunks.count(chunkPosition) != 0;
    }

    bool IsReadyToMesh(const glm::ivec2& chunkPosition) const;

    uint64_t GetMeshVersion(const glm::ivec2& chunkPosition) const
    {
        return m_Chunks.at(chunkPosition).meshVersion;
    }

    std::array<std::shared_ptr<const Chunk>, 9> Snapshot3x3(const glm::ivec2& center) const;

    static std::array<const Chunk*, 9> AsPointers(const std::array<std::shared_ptr<const Chunk>, 9>& grid);

    static constexpr bool IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius)
    {
        const glm::ivec2 d = entity - origin;
        return d.x * d.x + d.y * d.y <= radius * radius;
    }

private:
    struct ChunkRecord {
        std::shared_ptr<Chunk> chunk;

        uint64_t createdVersion;
        uint64_t lightVersion;
        uint64_t dispatchedLightVersion = 0;
        uint64_t litVersion = 0;

        uint64_t meshVersion;

        ChunkRecord(std::shared_ptr<Chunk> chunk, uint64_t version)
            : chunk(std::move(chunk))
            , createdVersion(version)
            , lightVersion(version)
            , meshVersion(version)
        {
        }

        bool IsLit() const
        {
            return litVersion != 0;
        }
    };

    struct TerrainResult {
        glm::ivec2 position;
        std::shared_ptr<Chunk> chunk;
        uint32_t generation;
    };

    struct LightResult {
        glm::ivec2 position;
        ChunkLight light;
        uint64_t version;
    };

    uint64_t NextVersion()
    {
        return m_NextVersion++;
    }

    void DrainResults();

    void Reload();

    bool WriteCell(const glm::ivec3& worldPosition, Block block, uint8_t fluid);

    void MarkChanged(const glm::ivec3& worldPosition, Block previous, Block block);

    bool UpdateLightIncrementally(const glm::ivec3& worldPosition, Block previous);

    void RemeshNeighboursSeeingChange(const Chunk& before, const Chunk& after);

    template <typename F>
    static void ForEachMeshSeeing(const glm::ivec3& cell, F&& f)
    {
        const glm::ivec2 chunk = ToChunkPosition(cell);
        const glm::ivec3 local = ToLocalPosition(cell);
        const int32_t minX = local.x == 0 ? -1 : 0;
        const int32_t maxX = local.x == Chunk::k_Width - 1 ? 1 : 0;
        const int32_t minZ = local.z == 0 ? -1 : 0;
        const int32_t maxZ = local.z == Chunk::k_Width - 1 ? 1 : 0;
        for (int32_t dz = minZ; dz <= maxZ; dz++) {
            for (int32_t dx = minX; dx <= maxX; dx++) {
                f(chunk + glm::ivec2(dx, dz));
            }
        }
    }

    void ProcessBlockUpdates();

    bool IsNeighbourhoodLoaded(const glm::ivec2& chunkPosition) const;

    void InvalidateChunk(const glm::ivec2& chunkPosition);
    void RemeshChunk(const glm::ivec2& chunkPosition);

    bool HasAllTerrainNeighbours(const glm::ivec2& chunkPosition) const;
    bool HasAllLitNeighbours(const glm::ivec2& chunkPosition) const;

    static constexpr size_t k_MaxBlockUpdatesPerTick = 16384;

    std::unordered_map<glm::ivec2, ChunkRecord> m_Chunks;
    int32_t m_RenderDistance = 10;

    float m_ChunkRemovalsPerSecond = 100.0f;
    float m_RemovalCredit = 0.0f;

    std::unordered_set<glm::ivec2> m_PendingTerrain;

    std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>> m_ModifiedChunks;

    std::deque<glm::ivec3> m_BlockUpdates;

    uint32_t m_Generation = 0;
    uint64_t m_NextVersion = 1;

    JobSystem& m_JobSystem;

    TerrainGenerator m_Generator;
    FallingBlockSystem m_FallingSystem;
    FluidSystem m_FluidSystem;
    DropSystem m_DropSystem;

    ResultQueue<TerrainResult> m_TerrainResults;
    ResultQueue<LightResult> m_LightResults;
};

}
