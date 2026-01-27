#pragma once

#include <cstdint>
#include <unordered_map>

#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "Krafter/World/Block.h"

namespace Krafter {

class Chunk {
public:
    static constexpr int32_t k_Width = 16;
    static constexpr int32_t k_Height = 256;

    Chunk(const glm::ivec2& position);
    Chunk(const Chunk& other);
    Chunk(Chunk&& other);
    ~Chunk();

    inline const glm::ivec2& GetPosition() const
    {
        return m_Position;
    }

    const Block& GetBlock(const glm::ivec3& coords) const;
    void SetBlock(const glm::ivec3& coords, Block value);

private:
    glm::ivec2 m_Position;
    Block* m_Blocks;
};

using ChunkMap = std::unordered_map<glm::ivec2, Chunk>;

} // namespace Krafter
