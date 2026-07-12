#include <algorithm>

#include "Krafter/World/Coords.h"
#include "Krafter/World/FallingBlockSystem.h"
#include "Krafter/World/World.h"

namespace Krafter {

FallingBlockSystem::FallingBlockSystem(World& world)
    : m_World(world)
{
}

void FallingBlockSystem::Schedule(const std::vector<glm::ivec3>& cells, const glm::ivec3& origin)
{
    for (const glm::ivec3& cell : cells) {
        if (!m_Falling.insert(cell).second) {
            continue;
        }
        const glm::ivec3 d = glm::abs(cell - origin);
        const int32_t dist = std::max({ d.x, d.y, d.z });
        m_FallingBlocks.push_back({ cell, static_cast<float>(dist) * k_FallStep });
    }
}

void FallingBlockSystem::Update(float deltaTime)
{
    if (m_FallingBlocks.empty()) {
        return;
    }

    std::unordered_set<glm::ivec2> touchedChunks;

    for (size_t i = 0; i < m_FallingBlocks.size();) {
        FallingBlock& falling = m_FallingBlocks[i];
        falling.delay -= deltaTime;
        if (falling.delay > 0.0f) {
            i++;
            continue;
        }

        const glm::ivec3 cell = falling.cell;
        const glm::ivec2 cellChunk = ToChunkPosition(cell);
        auto it = m_World.m_Chunks.find(cellChunk);

        const bool unloaded = it == m_World.m_Chunks.end() || !it->second.chunk;
        if (!unloaded && !m_World.CanEdit(cellChunk)) {
            i++;
            continue;
        }
        if (!unloaded) {
            const Block here = it->second.chunk->GetBlock(ToLocalPosition(cell));
            if (IsNaturalTreePart(here) || here == Block::k_Cactus) {
                it->second.chunk->SetBlock(ToLocalPosition(cell), Block::k_Air);
                touchedChunks.insert(cellChunk);

                m_World.SpawnDrop(glm::vec3(cell) + 0.5f, DropFor(here));
            }
        }

        m_Falling.erase(cell);
        m_FallingBlocks[i] = m_FallingBlocks.back();
        m_FallingBlocks.pop_back();
    }

    for (const glm::ivec2& chunkPosition : touchedChunks) {
        for (int32_t dz = -1; dz <= 1; dz++) {
            for (int32_t dx = -1; dx <= 1; dx++) {
                m_World.InvalidateChunk(chunkPosition + glm::ivec2(dx, dz));
            }
        }
    }
}

}
