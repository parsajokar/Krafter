#include <algorithm>
#include <atomic>

#include "Krafter/World/Chunk.h"

namespace Krafter {

namespace {

template <typename T>
T& Unshare(std::shared_ptr<T>& storage)
{
    // Only the main thread takes snapshots, so a count of 1 means no worker is still reading this.
    if (storage.use_count() != 1) {
        storage = std::make_shared<T>(*storage);
    } else {
        std::atomic_thread_fence(std::memory_order_acquire);
    }
    return *storage;
}

}

ChunkLight ChunkLight::Pack(const std::vector<uint8_t>& sky, const std::vector<uint8_t>& block)
{
    ChunkLight light;
    for (size_t s = 0; s < k_SectionCount; s++) {
        const size_t base = s * k_SectionVolume;
        const uint8_t first = PackLight(sky[base], block[base]);
        bool uniform = true;
        for (size_t i = 1; i < k_SectionVolume && uniform; i++) {
            uniform = PackLight(sky[base + i], block[base + i]) == first;
        }
        if (uniform) {
            light.uniform[s] = first;
            continue;
        }
        auto section = std::make_shared<Section>();
        for (size_t i = 0; i < k_SectionVolume; i++) {
            (*section)[i] = PackLight(sky[base + i], block[base + i]);
        }
        light.sections[s] = std::move(section);
    }
    return light;
}

Chunk::Chunk(const glm::ivec2& position)
    : m_Position(position)
{
}

Chunk::Section& Chunk::MutableSection(const glm::ivec3& coords)
{
    std::shared_ptr<Section>& section = m_Sections[coords.y / k_SectionHeight];
    if (!section) {
        section = std::make_shared<Section>();
        section->blocks.fill(Block::k_Air);
        section->fluid.fill(0);
        return *section;
    }
    return Unshare(section);
}

ChunkLight::Section& Chunk::MutableLightSection(const glm::ivec3& coords)
{
    const int32_t s = coords.y / k_SectionHeight;
    std::shared_ptr<ChunkLight::Section>& section = m_Light.sections[s];
    if (!section) {
        section = std::make_shared<ChunkLight::Section>();
        section->fill(m_Light.uniform[s]);
        return *section;
    }
    return Unshare(section);
}

Block Chunk::GetBlock(const glm::ivec3& coords) const
{
    const Section* section = m_Sections[coords.y / k_SectionHeight].get();
    return section ? section->blocks[SectionIndex(coords)] : Block::k_Air;
}

void Chunk::SetBlock(const glm::ivec3& coords, Block value)
{
    if (GetBlock(coords) == value) {
        return;
    }
    MutableSection(coords).blocks[SectionIndex(coords)] = value;
    m_Modified = true;
}

uint8_t Chunk::GetFluid(const glm::ivec3& coords) const
{
    const Section* section = m_Sections[coords.y / k_SectionHeight].get();
    return section ? section->fluid[SectionIndex(coords)] : 0;
}

void Chunk::SetFluid(const glm::ivec3& coords, uint8_t value)
{
    if (GetFluid(coords) == value) {
        return;
    }
    MutableSection(coords).fluid[SectionIndex(coords)] = value;
    m_Modified = true;
}

void Chunk::SetSkyLight(const glm::ivec3& coords, uint8_t value)
{
    if (GetSkyLight(coords) == value) {
        return;
    }
    uint8_t& packed = MutableLightSection(coords)[SectionIndex(coords)];
    packed = PackLight(value, packed & 0x0F);
}

void Chunk::SetBlockLight(const glm::ivec3& coords, uint8_t value)
{
    if (GetBlockLight(coords) == value) {
        return;
    }
    uint8_t& packed = MutableLightSection(coords)[SectionIndex(coords)];
    packed = PackLight(packed >> 4, value);
}

void Chunk::SetLight(ChunkLight light)
{
    m_Light = std::move(light);
}

int32_t Chunk::GetTopY() const
{
    for (int32_t s = k_SectionCount - 1; s >= 0; s--) {
        if (m_Sections[s]) {
            return (s + 1) * k_SectionHeight;
        }
    }
    return 0;
}

size_t Chunk::GetMemoryBytes() const
{
    size_t bytes = sizeof(Chunk);
    for (int32_t s = 0; s < k_SectionCount; s++) {
        bytes += m_Sections[s] ? sizeof(Section) : 0;
        bytes += m_Light.sections[s] ? sizeof(ChunkLight::Section) : 0;
    }
    return bytes;
}

}
