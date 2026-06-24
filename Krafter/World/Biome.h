#pragma once

#include <unordered_map>

#include "Krafter/World/Block.h"

namespace Krafter {

enum class BiomeType {
    k_Ocean,
    k_Plains,
    k_Desert
};

class Biome {
public:
    static void LoadBiomes();
    static const Biome& Get(BiomeType type);

    // Biome-shaping noise sampled at a world column. Shared by terrain
    // generation and any query (e.g. "which biome is the camera in?").
    static float Temperature(float worldX, float worldZ);
    static float Continentalness(float worldX, float worldZ);
    static BiomeType At(float worldX, float worldZ);
    static const char* Name(BiomeType type);

    // Continentalness picks ocean vs land; on land, temperature picks the biome.
    static BiomeType Select(float temperature, float continentalness);

    // Blends ocean and land (plains/desert) terrain so coastlines and biome
    // borders meet with a smooth slope instead of a sharp cliff.
    static int32_t SampleHeight(float temperature, float continentalness, float noiseValue);

    // Final terrain surface height at a world column, sampling all the noise
    // itself. Used by generation and by water features placing ponds.
    static int32_t SurfaceHeight(float worldX, float worldZ);

    Block surface;
    Block subsurface;
    int32_t subsurfaceDepth;

    int32_t baseHeight;
    int32_t heightAmplitude;

private:
    // Smoothstep weight toward the desert biome in [0, 1] for a temperature.
    static float DesertBlend(float temperature);

    // Smoothstep weight toward land in [0, 1]; 0 is open ocean, 1 is inland.
    static float Landness(float continentalness);

    inline static std::unordered_map<BiomeType, Biome> s_Biomes;
};

} // namespace Krafter
