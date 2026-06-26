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
    static void LoadBiomes();
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

    // Blends ocean and land (forest/desert) terrain so coastlines and biome
    // borders meet with a smooth slope instead of a sharp cliff.
    static int32_t SampleHeight(float temperature, float humidity, float continentalness, float noiseValue);

    // Final terrain surface height at a world column, sampling all the noise
    // itself. Used by generation and by water features placing ponds.
    static int32_t SurfaceHeight(float worldX, float worldZ);

    Block surface;
    Block subsurface;
    int32_t subsurfaceDepth;

    int32_t baseHeight;
    int32_t heightAmplitude;

    // Multiplied into the grayscale grass top/overlay so each biome greens its
    // grass differently (lush forest vs. dry desert).
    glm::vec3 grassColor;

    // Same idea for the grayscale leaf tile: each biome tints its foliage,
    // typically a shade deeper than its grass.
    glm::vec3 leafColor;

private:
    // Weight toward the desert biome in [0, 1] from the hot, dry corner of the
    // (temperature, humidity) climate plane. Used both to pick the surface block
    // and to blend terrain height, so the two always agree.
    static float Desertness(float temperature, float humidity);

    // Smoothstep weight toward land in [0, 1]; 0 is open ocean, 1 is inland.
    static float Landness(float continentalness);

    inline static std::unordered_map<BiomeType, Biome> s_Biomes;
};

} // namespace Krafter
