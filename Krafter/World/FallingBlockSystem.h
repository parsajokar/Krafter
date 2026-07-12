#pragma once

#include <cstdint>
#include <unordered_set>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

namespace Krafter {

class World;

class FallingBlockSystem {
public:
    explicit FallingBlockSystem(World& world);

    void Schedule(const std::vector<glm::ivec3>& cells, const glm::ivec3& origin);

    void Update(float deltaTime);

    bool IsFalling(const glm::ivec3& cell) const
    {
        return m_Falling.count(cell) != 0;
    }

private:
    struct FallingBlock {
        glm::ivec3 cell;
        float delay;
    };

    static constexpr float k_FallStep = 0.05f;

    World& m_World;

    std::vector<FallingBlock> m_FallingBlocks;
    std::unordered_set<glm::ivec3> m_Falling;
};

}
