#pragma once

#include <cstdint>

#include "glm/glm.hpp"

#include "Krafter/World/Block.h"

namespace Krafter {

class Chunk {
public:
    static constexpr int32_t k_Width = 16;
    static constexpr int32_t k_Height = 256;

    static constexpr int32_t k_SeaLevel = 63;

    Chunk(const glm::ivec2& position);
    Chunk(const Chunk& other);
    Chunk(Chunk&& other);
    ~Chunk();

    static void SetSeed(uint32_t seed);

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

    static constexpr uint8_t k_MaxLight = 15;

private:
    glm::ivec2 m_Position;
    Block* m_Blocks;
    uint8_t* m_SkyLight;
    uint8_t* m_BlockLight;
};

}
