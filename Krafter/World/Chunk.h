#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "glm/glm.hpp"

#include "Krafter/World/Block.h"

namespace Krafter {

struct ChunkLight {
    static constexpr size_t k_SectionCount = 16;
    static constexpr size_t k_SectionVolume = 16 * 16 * 16;

    using Section = std::array<uint8_t, k_SectionVolume>;

    std::array<std::shared_ptr<Section>, k_SectionCount> sections;
    std::array<uint8_t, k_SectionCount> uniform = {};

    static ChunkLight Pack(const std::vector<uint8_t>& sky, const std::vector<uint8_t>& block);
};

inline constexpr uint8_t PackLight(uint8_t sky, uint8_t block)
{
    return static_cast<uint8_t>((sky << 4) | block);
}

class Chunk {
public:
    static constexpr int32_t k_Width = 16;
    static constexpr int32_t k_Height = 256;
    static constexpr int32_t k_Volume = k_Width * k_Width * k_Height;

    static constexpr int32_t k_SectionHeight = 16;
    static constexpr int32_t k_SectionCount = k_Height / k_SectionHeight;
    static constexpr int32_t k_SectionVolume = k_Width * k_Width * k_SectionHeight;

    static constexpr int32_t k_SeaLevel = 63;

    explicit Chunk(const glm::ivec2& position);

    std::shared_ptr<const Chunk> Snapshot() const
    {
        return std::make_shared<const Chunk>(*this);
    }

    inline const glm::ivec2& GetPosition() const
    {
        return m_Position;
    }

    Block GetBlock(const glm::ivec3& coords) const;
    void SetBlock(const glm::ivec3& coords, Block value);

    uint8_t GetFluid(const glm::ivec3& coords) const;
    void SetFluid(const glm::ivec3& coords, uint8_t value);

    uint8_t GetSkyLight(const glm::ivec3& coords) const
    {
        return GetPackedLight(coords) >> 4;
    }
    uint8_t GetBlockLight(const glm::ivec3& coords) const
    {
        return GetPackedLight(coords) & 0x0F;
    }
    void SetSkyLight(const glm::ivec3& coords, uint8_t value);
    void SetBlockLight(const glm::ivec3& coords, uint8_t value);

    void SetLight(ChunkLight light);

    int32_t GetTopY() const;

    size_t GetMemoryBytes() const;

    bool IsModified() const
    {
        return m_Modified;
    }
    void ClearModified()
    {
        m_Modified = false;
    }

    static constexpr uint8_t k_MaxLight = 15;

    static constexpr int32_t Index(const glm::ivec3& coords)
    {
        return (coords.y * k_Width * k_Width) + (coords.z * k_Width) + coords.x;
    }

private:
    struct Section {
        std::array<Block, k_SectionVolume> blocks;
        std::array<uint8_t, k_SectionVolume> fluid;
    };

    static constexpr int32_t SectionIndex(const glm::ivec3& coords)
    {
        return ((coords.y % k_SectionHeight) * k_Width * k_Width) + (coords.z * k_Width) + coords.x;
    }

    uint8_t GetPackedLight(const glm::ivec3& coords) const
    {
        const int32_t s = coords.y / k_SectionHeight;
        const ChunkLight::Section* section = m_Light.sections[s].get();
        return section ? (*section)[SectionIndex(coords)] : m_Light.uniform[s];
    }

    Section& MutableSection(const glm::ivec3& coords);
    ChunkLight::Section& MutableLightSection(const glm::ivec3& coords);

    glm::ivec2 m_Position;
    std::array<std::shared_ptr<Section>, k_SectionCount> m_Sections;
    ChunkLight m_Light;
    bool m_Modified = false;
};

}
