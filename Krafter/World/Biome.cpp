#include <iostream>
#include <stdexcept>

#include "Krafter/World/Biome.h"

namespace Krafter {

void Biome::LoadBiomes()
{
    s_Biomes[BiomeType::k_Plains] = {
        .surface = Block::k_Grass,
        .subsurface = Block::k_Dirt,
        .subsurfaceDepth = 4
    };

    s_Biomes[BiomeType::k_Desert] = {
        .surface = Block::k_Sand,
        .subsurface = Block::k_Sand,
        .subsurfaceDepth = 4
    };
}

const Biome& Biome::Get(BiomeType type)
{
    try {
        return s_Biomes.at(type);
    } catch (std::out_of_range e) {
        std::cerr << "Biome is not defined!" << std::endl;
        throw e;
    }
}

BiomeType Biome::Select(float temperature)
{
    if (temperature > 0.0f) {
        return BiomeType::k_Desert;
    }
    return BiomeType::k_Plains;
}

} // namespace Krafter
