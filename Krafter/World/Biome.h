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

    // Blends terrain height parameters across the biome boundary so neighbouring
    // biomes meet with a smooth slope instead of a sharp cliff.
    static int32_t SampleHeight(float temperature, float noiseValue);

    Block surface;
    Block subsurface;
    int32_t subsurfaceDepth;

    int32_t baseHeight;
    int32_t heightAmplitude;

private:
    inline static std::unordered_map<BiomeType, Biome> s_Biomes;
};

} // namespace Krafter
