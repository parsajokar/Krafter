#include "imgui.h"

#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Window.h"
#include "Krafter/World/ChunkManager.h"

namespace Krafter {

ChunkManager::ChunkManager()
    : Layer("ChunkManager")
{
}

void ChunkManager::OnUpdate()
{
    glm::vec3 worldPosition = Renderer::Get()->GetCamera().GetPosition();
    glm::ivec2 chunkPosition = glm::ivec2(worldPosition.x, worldPosition.z) / Chunk::k_Width;

    for (int32_t x = chunkPosition.x - m_RenderDistance; x <= chunkPosition.x + m_RenderDistance; x++) {
        for (int32_t z = chunkPosition.y - m_RenderDistance; z <= chunkPosition.y + m_RenderDistance; z++) {
            glm::ivec2 position = glm::ivec2(x, z);
            if (IsInRadius(position, chunkPosition, m_RenderDistance)) {
                if (!m_ChunkMap.count(position) && !m_OnChunkGenerationQueue.count(position)) {
                    m_ChunkGenerationQueue.push_back(position);
                    m_OnChunkGenerationQueue[position] = true;
                }
            }
        }
    }

    for (const auto& [position, chunk] : m_ChunkMap) {
        if (!IsInRadius(position, chunkPosition, m_RenderDistance) && !m_OnChunkDeletionQueue.count(position)) {
            m_ChunkDeletionQueue.push_back(position);
            m_OnChunkDeletionQueue[position] = true;
        }
    }

    if (!m_ChunkGenerationQueue.empty() && Window::Get()->GetTime() > m_LastChunkGeneration + m_ChunkDelay) {
        m_LastChunkGeneration = Window::Get()->GetTime();

        glm::ivec2 targetChunk = m_ChunkGenerationQueue.front();
        m_ChunkGenerationQueue.pop_front();
        m_OnChunkGenerationQueue.erase(targetChunk);

        if (IsInRadius(targetChunk, chunkPosition, m_RenderDistance)) {
            m_ChunkMap.emplace(targetChunk, std::move(Chunk(targetChunk)));
            Renderer::Get()->GenerateChunkMesh(m_ChunkMap, targetChunk);
        }
    }

    if (!m_ChunkDeletionQueue.empty() && Window::Get()->GetTime() > m_LastChunkDeletion + m_ChunkDelay) {
        m_LastChunkDeletion = Window::Get()->GetTime();

        glm::ivec2 targetChunk = m_ChunkDeletionQueue.front();
        m_ChunkDeletionQueue.pop_front();
        m_OnChunkDeletionQueue.erase(targetChunk);

        if (!IsInRadius(targetChunk, chunkPosition, m_RenderDistance)) {
            m_ChunkMap.erase(targetChunk);
            Renderer::Get()->DeleteChunkMesh(targetChunk);
        }
    }
}

void ChunkManager::OnRenderImGui()
{
    ImGui::InputInt("Render Distance", &m_RenderDistance);
    ImGui::InputFloat("Chunk Delay", &m_ChunkDelay);
}

constexpr bool ChunkManager::IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius)
{
    glm::ivec2 d = entity - origin;
    return d.x * d.x + d.y * d.y <= radius * radius;
}

} // namespace Krafter
