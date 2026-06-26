#pragma once

#include <cstdint>

#include "glm/glm.hpp"

#include "Krafter/World/Block.h"

namespace Krafter {

class Chunk {
public:
    static constexpr int32_t k_Width = 16;
    static constexpr int32_t k_Height = 256;

    // Global level that oceans and rivers flood up to.
    static constexpr int32_t k_SeaLevel = 63;

    Chunk(const glm::ivec2& position);
    Chunk(const Chunk& other);
    Chunk(Chunk&& other);
    ~Chunk();

    // Sets the world seed mixed into feature placement (trees, plants, lakes).
    // Call once before any chunk is generated.
    static void SetSeed(uint32_t seed);

    inline const glm::ivec2& GetPosition() const
    {
        return m_Position;
    }

    const Block& GetBlock(const glm::ivec3& coords) const;
    void SetBlock(const glm::ivec3& coords, Block value);

    // Sky-light level in [0, 15]. Computed by the lighting pass once the chunk
    // and its neighbours have terrain, then sampled by the mesher.
    uint8_t GetSkyLight(const glm::ivec3& coords) const;
    void SetSkyLight(const glm::ivec3& coords, uint8_t value);

    static constexpr uint8_t k_MaxLight = 15;

private:
    glm::ivec2 m_Position;
    Block* m_Blocks;
    uint8_t* m_SkyLight;
};

} // namespace Krafter
