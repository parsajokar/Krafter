#include <iostream>
#include <stdexcept>

#include "block.h"

namespace Krafter
{

void BlockAtlas::LoadAtlases()
{
    _blockAtlases[Block::DIRT] = {
        .top = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f)
    };

    _blockAtlases[Block::GRASS] = {
        .top = glm::vec2(2.0f / 16.0f, 0.0f / 16.0f),
        .side = glm::vec2(1.0f / 16.0f, 0.0f / 16.0f),
        .bottom = glm::vec2(0.0f / 16.0f, 0.0f / 16.0f)
    };
}

const BlockAtlas& BlockAtlas::GetAtlasOf(Block block)
{
    try {
        return _blockAtlases.at(block);
    } catch (std::out_of_range e) {
        std::cerr << "Block atlas is not defined!" << std::endl;
        throw e;
    }
}

Chunk::Chunk(const glm::ivec2& position)
    : _position(position)
{
    _blocks = new Block[WIDTH * WIDTH * HEIGHT];

    for (int32_t y = HEIGHT - 1; y >= 0; y--) {
        for (int32_t x = 0; x < WIDTH; x++) {
            for (int32_t z = 0; z < WIDTH; z++) {
                if ((x - 8.5f) * (x - 8.5f) + (y % 16 - 8.5f) * (y % 16 - 8.5f) + (z - 8.5f) * (z - 8.5f) <= 8.5f * 8.5f) {
                    if (y + 1 >= HEIGHT || GetBlock(glm::ivec3(x, y + 1, z)) == Block::AIR) {
                        SetBlock(glm::ivec3(x, y, z), Block::GRASS);
                    } else {
                        SetBlock(glm::ivec3(x, y, z), Block::DIRT);
                    }
                } else {
                    SetBlock(glm::ivec3(x, y, z), Block::AIR);
                }
            }
        }
    }
}

Chunk::Chunk(const Chunk& other)
    : _position(other._position)
{
    std::cout << "Copying chunk at position (" << other._position.x << ", " << other._position.y << ")" << std::endl;

    _blocks = new Block[WIDTH * WIDTH * HEIGHT];
    for (uint32_t i = 0; i < WIDTH * WIDTH * HEIGHT; i++) {
        _blocks[i] = other._blocks[i];
    }
}

Chunk::Chunk(Chunk&& other)
    : _position(other._position)
    , _blocks(other._blocks)
{
    other._blocks = nullptr;
}

Chunk::~Chunk()
{
    delete[] _blocks;
}

const Block& Chunk::GetBlock(const glm::ivec3& coords) const
{
    return _blocks[(coords.y * WIDTH * WIDTH) + (coords.z * WIDTH) + coords.x];
}

void Chunk::SetBlock(const glm::ivec3& coords, Block value)
{
    _blocks[(coords.y * WIDTH * WIDTH) + (coords.z * WIDTH) + coords.x] = value;
}

} // namespace Krafter
