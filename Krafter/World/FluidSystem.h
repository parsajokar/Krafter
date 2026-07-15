#pragma once

#include <cstdint>
#include <unordered_set>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "Krafter/World/Block.h"

namespace Krafter {

class Chunk;
class World;

class FluidSystem {
public:
    explicit FluidSystem(World& world);

    void Schedule(const glm::ivec3& cell, Block fluid);

    void Update(float deltaTime);

private:
    static constexpr float k_WaterTick = 0.25f;
    static constexpr float k_LavaTick = 1.0f;

    void Tick(Block fluid, std::unordered_set<glm::ivec3>& pending);
    void Evaluate(const glm::ivec3& cell, Block fluid,
        std::unordered_set<glm::ivec3>& pending,
        std::unordered_set<glm::ivec2>& remesh, std::unordered_set<glm::ivec2>& relight);

    Chunk* ChunkAt(const glm::ivec3& cell) const;
    Block BlockAt(const glm::ivec3& cell) const;
    uint8_t FluidAt(const glm::ivec3& cell) const;

    World& m_World;

    std::unordered_set<glm::ivec3> m_PendingWater;
    std::unordered_set<glm::ivec3> m_PendingLava;

    float m_WaterTimer = 0.0f;
    float m_LavaTimer = 0.0f;

    uint32_t m_Tick = 0;
};

}
