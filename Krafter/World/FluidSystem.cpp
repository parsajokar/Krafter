#include <algorithm>

#include "Krafter/World/Coords.h"
#include "Krafter/World/Fluid.h"
#include "Krafter/World/FluidSystem.h"
#include "Krafter/World/WorldAccess.h"

namespace Krafter {

namespace {

constexpr glm::ivec3 k_Up(0, 1, 0);

constexpr glm::ivec3 k_Neighbors[6] = {
    { 1, 0, 0 }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 0, -1 }, { 0, 1, 0 }, { 0, -1, 0 }
};

}

FluidSystem::FluidSystem(WorldAccess& world)
    : m_World(world)
{
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

    for (const glm::ivec3& cell : snapshot) {
        if (!m_World.IsLoadedAround(cell)) {
            if (m_World.IsLoadedOrKept(cell)) {
                pending.insert(cell);
            }
            continue;
        }
        Evaluate(cell, fluid, pending);
    }
}

void FluidSystem::Evaluate(const glm::ivec3& cell, Block fluid, std::unordered_set<glm::ivec3>& pending)
{
    auto blockAt = [&](const glm::ivec3& c) { return m_World.GetBlock(c); };
    auto fluidAt = [&](const glm::ivec3& c) { return m_World.GetFluid(c); };

    if (blockAt(cell) != fluid) {
        return;
    }

    auto feedAt = [&](const glm::ivec3& c) -> int32_t {
        if (blockAt(c) != fluid) {
            return 0;
        }
        const uint8_t f = fluidAt(c);
        if (FluidIsSource(f) || blockAt(c + k_Up) == fluid) {
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
            const Block b = blockAt(n);
            if (b == Block::k_Air || b == fluid) {
                pending.insert(n);
            }
        }
    };
    auto setLevel = [&](const glm::ivec3& c, uint8_t value) {
        if (value == 0) {
            m_World.SetBlock(c, Block::k_Air);
        } else {
            m_World.SetFluid(c, fluid, value);
        }
        scheduleAround(c);
    };

    const Block other = fluid == Block::k_Water ? Block::k_Lava : Block::k_Water;
    for (const glm::ivec3& dir : k_Neighbors) {
        const glm::ivec3 n = cell + dir;
        if (blockAt(n) != other) {
            continue;
        }
        if (fluid == Block::k_Lava) {
            m_World.SetBlock(cell, Block::k_Stone);
            scheduleAround(cell);
            return;
        }
        m_World.SetBlock(n, Block::k_Stone);
    }

    const uint8_t self = fluidAt(cell);
    const bool source = FluidIsSource(self);
    const bool fedFromAbove = blockAt(cell + k_Up) == fluid;
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
            m_World.SetFluid(cell, fluid, static_cast<uint8_t>(level));
            scheduleAround(cell);
        }
    }

    const glm::ivec3 below = cell - k_Up;
    if (below.y >= 0) {
        const Block belowBlock = blockAt(below);
        const bool canFall = belowBlock == Block::k_Air
            || (belowBlock == fluid && !FluidIsSource(fluidAt(below))
                && FluidLevel(fluidAt(below)) < k_FluidFull);
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
            const Block b = blockAt(n);
            const bool canReceive = b == Block::k_Air
                || (b == fluid && !FluidIsSource(fluidAt(n))
                    && FluidLevel(fluidAt(n)) < spread);
            if (canReceive) {
                setLevel(n, static_cast<uint8_t>(spread));
            }
        }
    }
}

}
