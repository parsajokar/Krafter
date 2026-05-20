#include "imgui.h"

#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Window.h"
#include "Krafter/World/World.h"

namespace Krafter {

void World::Update()
{
    glm::vec3 worldPosition = Renderer::GetCamera()->GetPosition();
    glm::ivec2 chunkPosition = glm::ivec2(worldPosition.x, worldPosition.z) / Chunk::k_Width;

    for (int32_t x = chunkPosition.x - m_RenderDistance; x <= chunkPosition.x + m_RenderDistance; x++) {
        for (int32_t z = chunkPosition.y - m_RenderDistance; z <= chunkPosition.y + m_RenderDistance; z++) {
            glm::ivec2 position = glm::ivec2(x, z);
            if (IsInRadius(position, chunkPosition, m_RenderDistance)) {
                if (!m_ChunkMeshMap.count(position) && !m_OnChunkGenerationQueue.count(position)) {
                    m_ChunkGenerationQueue.push_back(position);
                    m_OnChunkGenerationQueue[position] = true;
                }
            }
        }
    }

    for (const auto& [position, chunkMesh] : m_ChunkMeshMap) {
        if (!IsInRadius(position, chunkPosition, m_RenderDistance) && !m_OnChunkDeletionQueue.count(position)) {
            m_ChunkDeletionQueue.push_back(position);
            m_OnChunkDeletionQueue[position] = true;
        }
    }

    if (!m_ChunkGenerationQueue.empty() && Window::GetTime() > m_LastChunkGeneration + m_ChunkDelay) {
        m_LastChunkGeneration = Window::GetTime();

        glm::ivec2 targetChunk = m_ChunkGenerationQueue.front();
        m_ChunkGenerationQueue.pop_front();
        m_OnChunkGenerationQueue.erase(targetChunk);

        if (IsInRadius(targetChunk, chunkPosition, m_RenderDistance)) {
            LoadChunk(targetChunk);
        }
    }

    if (!m_ChunkDeletionQueue.empty() && Window::GetTime() > m_LastChunkDeletion + m_ChunkDelay) {
        m_LastChunkDeletion = Window::GetTime();

        glm::ivec2 targetChunk = m_ChunkDeletionQueue.front();
        m_ChunkDeletionQueue.pop_front();
        m_OnChunkDeletionQueue.erase(targetChunk);

        if (!IsInRadius(targetChunk, chunkPosition, m_RenderDistance)) {
            UnloadChunk(targetChunk);
        }
    }
}

void World::Render()
{
    for (const auto& [position, chunkMesh] : m_ChunkMeshMap) {
        Renderer::RenderChunkMesh(*chunkMesh);
    }
}

void World::RenderImGui()
{
    ImGui::InputInt("Render Distance", &m_RenderDistance);
    ImGui::InputFloat("Chunk Delay", &m_ChunkDelay);
}

Block World::GetBlock(const glm::ivec3& worldPosition) const
{
    if (worldPosition.y < 0 || worldPosition.y >= Chunk::k_Height) {
        return Block::k_Air;
    }

    auto floorDiv = [](int32_t a, int32_t b) {
        int32_t q = a / b;
        if ((a % b != 0) && ((a < 0) != (b < 0))) {
            q--;
        }
        return q;
    };
    auto floorMod = [](int32_t a, int32_t b) {
        int32_t r = a % b;
        if (r != 0 && (r < 0) != (b < 0)) {
            r += b;
        }
        return r;
    };

    glm::ivec2 chunkPosition(
        floorDiv(worldPosition.x, Chunk::k_Width),
        floorDiv(worldPosition.z, Chunk::k_Width));
    glm::ivec3 localPosition(
        floorMod(worldPosition.x, Chunk::k_Width),
        worldPosition.y,
        floorMod(worldPosition.z, Chunk::k_Width));

    auto it = m_ChunkMap.find(chunkPosition);
    if (it == m_ChunkMap.end()) {
        return Block::k_Air;
    }
    return it->second.GetBlock(localPosition);
}

constexpr bool World::IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius)
{
    glm::ivec2 d = entity - origin;
    return d.x * d.x + d.y * d.y <= radius * radius;
}

void World::LoadChunk(const glm::ivec2& chunkPosition)
{
    constexpr glm::ivec2 dp[] = {
        glm::ivec2(-1, 0),
        glm::ivec2(1, 0),
        glm::ivec2(0, -1),
        glm::ivec2(0, 1)
    };

    for (size_t i = 0; i < 4; i++) {
        glm::ivec2 neighbourChunk = chunkPosition + dp[i];
        if (!IsChunkLoadedToChunkMap(neighbourChunk)) {
            LoadChunkToChunkMap(neighbourChunk);
        }
    }

    if (!IsChunkLoadedToChunkMap(chunkPosition)) {
        LoadChunkToChunkMap(chunkPosition);
    }


    m_ChunkMeshMap[chunkPosition] = std::make_unique<ChunkMesh>(*this, chunkPosition);
}

void World::UnloadChunk(const glm::ivec2& chunkPosition)
{
    m_ChunkMeshMap.erase(chunkPosition);

}

bool World::IsChunkLoadedToChunkMap(const glm::ivec2& chunkPosition) const
{
    return m_ChunkMap.count(chunkPosition) != 0;
}

void World::LoadChunkToChunkMap(const glm::ivec2& chunkPosition)
{
    m_ChunkMap.emplace(chunkPosition, std::move(Chunk(chunkPosition)));
}

} // namespace Krafter
