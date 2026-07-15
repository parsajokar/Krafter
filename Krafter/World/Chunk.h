#pragma once

#include <cstdint>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/World/Block.h"

namespace Krafter {

class Chunk {
public:
    static constexpr int32_t k_Width = 16;
    static constexpr int32_t k_Height = 256;

    static constexpr int32_t k_SeaLevel = 63;

    Chunk(const glm::ivec2& position);

    inline const glm::ivec2& GetPosition() const
    {
        return m_Position;
    }

    const Block& GetBlock(const glm::ivec3& coords) const;
    void SetBlock(const glm::ivec3& coords, Block value);

    uint8_t GetSkyLight(const glm::ivec3& coords) const;
    void SetSkyLight(const glm::ivec3& coords, uint8_t value);

    uint8_t GetBlockLight(const glm::ivec3& coords) const;
    void SetBlockLight(const glm::ivec3& coords, uint8_t value);

    // Flow metadata for the cell's fluid (see Fluid.h). Zero for non-fluid cells,
    // which also happens to decode as a source so generated fluids need no setup.
    uint8_t GetFluid(const glm::ivec3& coords) const;
    void SetFluid(const glm::ivec3& coords, uint8_t value);

    static constexpr uint8_t k_MaxLight = 15;

private:
    glm::ivec2 m_Position;
    std::vector<Block> m_Blocks;
    std::vector<uint8_t> m_SkyLight;
    std::vector<uint8_t> m_BlockLight;
    std::vector<uint8_t> m_Fluid;
};

}
