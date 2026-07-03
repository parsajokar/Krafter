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
    // Configures the terrain for a world seed: seeds the climate/detail noise and
    // loads the biome table. Call once before any chunk is generated.
    static void Configure(int32_t seed);
    static const Biome& Get(BiomeType type);

    // Biome-shaping noise sampled at a world column. Shared by terrain
    // generation and any query (e.g. "which biome is the camera in?").
    static float Temperature(float worldX, float worldZ);
    static float Humidity(float worldX, float worldZ);
    static float Continentalness(float worldX, float worldZ);
    static BiomeType At(float worldX, float worldZ);
    static const char* Name(BiomeType type);

    // Continentalness picks ocean vs land; on land, temperature and humidity
    // together pick the biome (the multi-noise climate model).
    static BiomeType Select(float temperature, float humidity, float continentalness);

    // Final terrain surface height at a world column, sampling all the noise
    // itself. Height is a function of continentalness, erosion, and a
    // peaks-and-valleys fold of weirdness — independent of the chosen biome, the
    // way 1.18 separates terrain shape from climate. Used by generation and by
    // water features placing ponds.
    static int32_t SurfaceHeight(float worldX, float worldZ);

    Block surface;
    Block subsurface;
    int32_t subsurfaceDepth;

    // Multiplied into the grayscale grass top/overlay so each biome greens its
    // grass differently (lush forest vs. dry desert).
    glm::vec3 grassColor;

    // Same idea for the grayscale leaf tile: each biome tints its foliage,
    // typically a shade deeper than its grass.
    glm::vec3 leafColor;

private:
    // Populates the biome table. Split from Configure so the seed setup and the
    // (seed-independent) biome definitions stay separate.
    static void LoadBiomes();

    // Smoothstep weight toward land in [0, 1]; 0 is open ocean, 1 is inland.
    static float Landness(float continentalness);

    inline static std::unordered_map<BiomeType, Biome> s_Biomes;
};

} // namespace Krafter
