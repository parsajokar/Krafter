#include <algorithm>

#include "Krafter/World/FallingBlockSystem.h"
#include "Krafter/World/WorldAccess.h"

namespace Krafter {

FallingBlockSystem::FallingBlockSystem(WorldAccess& world)
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
    for (size_t i = 0; i < m_FallingBlocks.size();) {
        FallingBlock& falling = m_FallingBlocks[i];
        falling.delay -= deltaTime;
        if (falling.delay > 0.0f) {
            i++;
            continue;
        }

        const glm::ivec3 cell = falling.cell;
        m_FallingBlocks[i] = m_FallingBlocks.back();
        m_FallingBlocks.pop_back();
        m_Falling.erase(cell);

        const Block here = m_World.GetBlock(cell);
        if ((IsNaturalTreePart(here) || here == Block::k_Cactus) && m_World.SetBlock(cell, Block::k_Air)) {
            m_World.SpawnDrop(glm::vec3(cell) + 0.5f, DropItemFor(here));
        }
    }
}

}
