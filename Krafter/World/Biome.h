#pragma once

#include <unordered_map>

#include "Krafter/World/Block.h"

namespace Krafter {

enum class BiomeType {
    k_Ocean,
    k_OakForest,
    k_BirchForest,
    k_Savannah,
    k_Desert
};

class Biome {
public:
    static void Configure(int32_t seed);
    static const Biome& Get(BiomeType type);

    static float Temperature(float worldX, float worldZ);
    static float Humidity(float worldX, float worldZ);
    static float Continentalness(float worldX, float worldZ);
    static BiomeType At(float worldX, float worldZ);
    static const char* Name(BiomeType type);

    static BiomeType Select(float temperature, float humidity, float continentalness);

    static int32_t SurfaceHeight(float worldX, float worldZ);

    Block surface;
    Block subsurface;
    int32_t subsurfaceDepth;

    glm::vec3 grassColor;

    glm::vec3 leafColor;

private:
    static void LoadBiomes();

    static float Landness(float continentalness);

    inline static std::unordered_map<BiomeType, Biome> s_Biomes;
};

}
