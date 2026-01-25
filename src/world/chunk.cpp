#include <cstring>
#include <iostream>

#include "FastNoiseLite.h"

#include "world/chunk.h"

namespace Krafter {

Chunk::Chunk(const glm::ivec2& position)
    : _position(position)
{
    _blocks = new Block[WIDTH * WIDTH * HEIGHT];
    memset(_blocks, 0, WIDTH * WIDTH * HEIGHT * sizeof(Block));

    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(0.01f);

    for (int32_t x = 0; x < WIDTH; x++) {
        for (int32_t z = 0; z < WIDTH; z++) {
            const int32_t worldX = _position.x * WIDTH + x;
            const int32_t worldZ = _position.y * WIDTH + z;

            float noiseValue = noise.GetNoise((float)worldX, (float)worldZ); // Range = [-1, 1]
            int32_t height = (int32_t)(32 * noiseValue) + 64; // Range = [32, 96]

            for (int32_t y = 0; y < height; y++) {
                SetBlock(glm::ivec3(x, y, z), Block::DIRT);
            }

            SetBlock(glm::ivec3(x, height, z), Block::GRASS);
        }
    }
}

Chunk::Chunk(const Chunk& other)
    : _position(other._position)
{
    std::cout << "[CHUNK] Copying chunk at position (" << other._position.x << ", " << other._position.y << ")" << std::endl;

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
