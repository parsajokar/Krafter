#pragma once

#include <cstdint>

#include "Krafter/World/Block.h"

namespace Krafter {

inline constexpr bool IsFluid(Block block)
{
    return block == Block::k_Water || block == Block::k_Lava;
}

constexpr uint8_t k_FluidLevelMask = 0x0F;
constexpr uint8_t k_FluidSourceFlag = 0x40;
constexpr uint8_t k_FluidFull = 8;
constexpr uint8_t k_FluidSource = k_FluidSourceFlag | k_FluidFull;

constexpr float k_FluidSurface = 0.875f;

inline constexpr uint8_t FluidLevel(uint8_t fluid) { return fluid & k_FluidLevelMask; }

inline constexpr bool FluidIsSource(uint8_t fluid) { return (fluid & k_FluidSourceFlag) != 0; }

inline constexpr float FluidHeight(uint8_t fluid)
{
    return static_cast<float>(fluid & k_FluidLevelMask) / static_cast<float>(k_FluidFull) * k_FluidSurface;
}

}
