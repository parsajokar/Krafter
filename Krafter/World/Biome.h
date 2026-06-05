#pragma once

#include <unordered_map>

#include "Krafter/World/Block.h"

namespace Krafter {

enum class BiomeType {
    k_Plains,
    k_Desert
};

class Biome {
public:
    static void LoadBiomes();
    static const Biome& Get(BiomeType type);

    static BiomeType Select(float temperature);

    Block surface;
    Block subsurface;
    int32_t subsurfaceDepth;

private:
    inline static std::unordered_map<BiomeType, Biome> s_Biomes;
};

} // namespace Krafter
