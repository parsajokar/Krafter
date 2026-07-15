#include <algorithm>

#include "Krafter/World/Coords.h"
#include "Krafter/World/Fluid.h"
#include "Krafter/World/FluidSystem.h"
#include "Krafter/World/World.h"

namespace Krafter {

namespace {

constexpr glm::ivec3 k_Up(0, 1, 0);

constexpr glm::ivec3 k_Neighbors[6] = {
    { 1, 0, 0 }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0, 1, 0 }, { 0, -1, 0 }
};

}

FluidSystem::FluidSystem(World& world)
    : m_World(world)
{
}

Chunk* FluidSystem::ChunkAt(const glm::ivec3& cell) const
{
    auto it = m_World.m_Chunks.find(ToChunkPosition(cell));
    if (it == m_World.m_Chunks.end()) {
        return nullptr;
    }
    return it->second.chunk.get();
}

Block FluidSystem::BlockAt(const glm::ivec3& cell) const
{
    if (cell.y < 0 || cell.y >= Chunk::k_Height) {
        return Block::k_Air;
    }
    Chunk* chunk = ChunkAt(cell);
    return chunk ? chunk->GetBlock(ToLocalPosition(cell)) : Block::k_Air;
}

uint8_t FluidSystem::FluidAt(const glm::ivec3& cell) const
{
    if (cell.y < 0 || cell.y >= Chunk::k_Height) {
        return 0;
    }
    Chunk* chunk = ChunkAt(cell);
    return chunk ? chunk->GetFluid(ToLocalPosition(cell)) : 0;
}

void FluidSystem::Schedule(const glm::ivec3& cell, Block fluid)
{
    (fluid == Block::k_Lava ? m_PendingLava : m_PendingWater).insert(cell);
}

void FluidSystem::Update(float deltaTime)
{
    if (!m_PendingWater.empty()) {
        m_WaterTimer += deltaTime;
        if (m_WaterTimer >= k_WaterTick) {
            m_WaterTimer = 0.0f;
            Tick(Block::k_Water, m_PendingWater);
        }
    } else {
        m_WaterTimer = 0.0f;
    }

    if (!m_PendingLava.empty()) {
        m_LavaTimer += deltaTime;
        if (m_LavaTimer >= k_LavaTick) {
            m_LavaTimer = 0.0f;
            Tick(Block::k_Lava, m_PendingLava);
        }
    } else {
        m_LavaTimer = 0.0f;
    }
}

void FluidSystem::Tick(Block fluid, std::unordered_set<glm::ivec3>& pending)
{
    m_Tick++;

    std::unordered_set<glm::ivec3> snapshot;
    snapshot.swap(pending);

    std::unordered_set<glm::ivec2> remesh;
    std::unordered_set<glm::ivec2> relight;

    for (const glm::ivec3& cell : snapshot) {
        const glm::ivec2 chunkPos = ToChunkPosition(cell);
        auto it = m_World.m_Chunks.find(chunkPos);
        if (it == m_World.m_Chunks.end() || !it->second.chunk) {
            continue;
        }
        if (!m_World.CanEdit(chunkPos)) {
            pending.insert(cell);
            continue;
        }
        Evaluate(cell, fluid, pending, remesh, relight);
    }

    const bool lava = fluid == Block::k_Lava;
    for (const glm::ivec2& chunkPos : remesh) {
        if (lava || relight.count(chunkPos)) {
            m_World.InvalidateChunk(chunkPos);
        } else {
            m_World.RemeshChunk(chunkPos);
        }
    }
    for (const glm::ivec2& chunkPos : relight) {
        if (!remesh.count(chunkPos)) {
            m_World.InvalidateChunk(chunkPos);
        }
    }
}

void FluidSystem::Evaluate(const glm::ivec3& cell, Block fluid,
    std::unordered_set<glm::ivec3>& pending,
    std::unordered_set<glm::ivec2>& remesh, std::unordered_set<glm::ivec2>& relight)
{
    Chunk* chunk = ChunkAt(cell);
    const glm::ivec3 local = ToLocalPosition(cell);
    if (chunk->GetBlock(local) != fluid) {
        return;
    }

    auto markTouched = [&](std::unordered_set<glm::ivec2>& set, const glm::ivec3& c) {
        const glm::ivec2 cp = ToChunkPosition(c);
        set.insert(cp);
        const glm::ivec3 lc = ToLocalPosition(c);
        if (lc.x == 0) {
            set.insert(cp + glm::ivec2(-1, 0));
        }
        if (lc.x == Chunk::k_Width - 1) {
            set.insert(cp + glm::ivec2(1, 0));
        }
        if (lc.z == 0) {
            set.insert(cp + glm::ivec2(0, -1));
        }
        if (lc.z == Chunk::k_Width - 1) {
            set.insert(cp + glm::ivec2(0, 1));
        }
    };

    auto feedAt = [&](const glm::ivec3& c) -> int32_t {
        if (BlockAt(c) != fluid) {
            return 0;
        }
        const uint8_t f = FluidAt(c);
        if (FluidIsSource(f) || BlockAt(c + k_Up) == fluid) {
            return k_FluidFull;
        }
        return FluidLevel(f);
    };
    auto scheduleAround = [&](const glm::ivec3& c) {
        pending.insert(c);
        for (const glm::ivec3& dir : k_Neighbors) {
            const glm::ivec3 n = c + dir;
            if (n.y < 0 || n.y >= Chunk::k_Height) {
                continue;
            }
            const Block b = BlockAt(n);
            if (b == Block::k_Air || b == fluid) {
                pending.insert(n);
            }
        }
    };
    auto setLevel = [&](const glm::ivec3& c, uint8_t value) {
        Chunk* target = ChunkAt(c);
        const glm::ivec3 lc = ToLocalPosition(c);
        if (value == 0) {
            target->SetBlock(lc, Block::k_Air);
            target->SetFluid(lc, 0);
        } else {
            target->SetBlock(lc, fluid);
            target->SetFluid(lc, value);
        }
        markTouched(remesh, c);
        scheduleAround(c);
    };

    const Block other = fluid == Block::k_Water ? Block::k_Lava : Block::k_Water;
    for (const glm::ivec3& dir : k_Neighbors) {
        const glm::ivec3 n = cell + dir;
        if (BlockAt(n) != other) {
            continue;
        }
        if (fluid == Block::k_Lava) {
            chunk->SetBlock(local, Block::k_Stone);
            chunk->SetFluid(local, 0);
            markTouched(relight, cell);
            scheduleAround(cell);
            return;
        }
        Chunk* neighbourChunk = ChunkAt(n);
        neighbourChunk->SetBlock(ToLocalPosition(n), Block::k_Stone);
        neighbourChunk->SetFluid(ToLocalPosition(n), 0);
        markTouched(relight, n);
    }

    const uint8_t self = chunk->GetFluid(local);
    const bool source = FluidIsSource(self);
    const bool fedFromAbove = BlockAt(cell + k_Up) == fluid;
    const int32_t decrement = fluid == Block::k_Lava ? 2 : 1;

    int32_t level = FluidLevel(self);
    if (!source) {
        int32_t desired = fedFromAbove ? k_FluidFull : 0;
        if (!fedFromAbove) {
            for (const glm::ivec3& side : k_HorizontalNeighbors) {
                desired = std::max(desired, feedAt(cell + side) - decrement);
            }
        }
        if (desired <= 0) {
            setLevel(cell, 0);
            return;
        }
        if (desired != level) {
            level = desired;
            chunk->SetFluid(local, static_cast<uint8_t>(level));
            markTouched(remesh, cell);
            scheduleAround(cell);
        }
    }

    const glm::ivec3 below = cell - k_Up;
    if (below.y >= 0) {
        const Block belowBlock = BlockAt(below);
        const bool canFall = belowBlock == Block::k_Air
            || (belowBlock == fluid && !FluidIsSource(FluidAt(below))
                && FluidLevel(FluidAt(below)) < k_FluidFull);
        if (canFall) {
            setLevel(below, k_FluidFull);
            return;
        }
    }

    const int32_t feed = (source || fedFromAbove) ? k_FluidFull : level;
    const int32_t spread = feed - decrement;
    if (spread >= 1) {
        for (const glm::ivec3& side : k_HorizontalNeighbors) {
            const glm::ivec3 n = cell + side;
            const Block b = BlockAt(n);
            const bool canReceive = b == Block::k_Air
                || (b == fluid && !FluidIsSource(FluidAt(n))
                    && FluidLevel(FluidAt(n)) < spread);
            if (canReceive) {
                setLevel(n, static_cast<uint8_t>(spread));
            }
        }
    }
}

}
