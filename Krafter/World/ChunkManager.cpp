#include "imgui.h"

#include "Krafter/Renderer/Renderer.h"
#include "Krafter/Window.h"
#include "Krafter/World/ChunkManager.h"

namespace Krafter {

ChunkManager::ChunkManager()
    : Layer("ChunkManager")
{
}

static bool IsInRadius(const glm::ivec2& entity, const glm::ivec2& origin, int32_t radius)
{
    glm::ivec2 d = entity - origin;
    return d.x * d.x + d.y * d.y <= radius * radius;
}

void ChunkManager::OnUpdate()
{
    glm::vec3 worldPosition = Renderer::Get()->GetCamera().GetPosition();
    glm::ivec2 chunkPosition = glm::ivec2(worldPosition.x, worldPosition.z) / Chunk::k_Width;

    for (int32_t x = chunkPosition.x - m_renderDistance; x <= chunkPosition.x + m_renderDistance; x++) {
        for (int32_t z = chunkPosition.y - m_renderDistance; z <= chunkPosition.y + m_renderDistance; z++) {
            glm::ivec2 position = glm::ivec2(x, z);
            if (IsInRadius(position, chunkPosition, m_renderDistance)) {
                if (!m_chunkMap.count(position) && !m_onChunkGenerationQueue.count(position)) {
                    m_chunkGenerationQueue.push_back(position);
                    m_onChunkGenerationQueue[position] = true;
                }
            }
        }
    }

    for (const auto& [position, chunk] : m_chunkMap) {
        if (!IsInRadius(position, chunkPosition, m_renderDistance) && !m_onChunkDeletionQueue.count(position)) {
            m_chunkDeletionQueue.push_back(position);
            m_onChunkDeletionQueue[position] = true;
        }
    }

    if (!m_chunkGenerationQueue.empty() && Window::Get()->GetTime() > m_lastChunkGeneration + m_chunkDelay) {
        m_lastChunkGeneration = Window::Get()->GetTime();

        glm::ivec2 targetChunk = m_chunkGenerationQueue.front();
        m_chunkGenerationQueue.pop_front();
        m_onChunkGenerationQueue.erase(targetChunk);

        if (IsInRadius(targetChunk, chunkPosition, m_renderDistance)) {
            m_chunkMap.emplace(targetChunk, std::move(Chunk(targetChunk)));
            Renderer::Get()->GenerateChunkMesh(m_chunkMap, targetChunk);
        }
    }

    if (!m_chunkDeletionQueue.empty() && Window::Get()->GetTime() > m_lastChunkDeletion + m_chunkDelay) {
        m_lastChunkDeletion = Window::Get()->GetTime();

        glm::ivec2 targetChunk = m_chunkDeletionQueue.front();
        m_chunkDeletionQueue.pop_front();
        m_onChunkDeletionQueue.erase(targetChunk);

        if (!IsInRadius(targetChunk, chunkPosition, m_renderDistance)) {
            m_chunkMap.erase(targetChunk);
            Renderer::Get()->DeleteChunkMesh(targetChunk);
        }
    }
}

void ChunkManager::OnRenderImGui()
{
    ImGui::InputInt("Render Distance", &m_renderDistance);
    ImGui::InputFloat("Chunk Delay", &m_chunkDelay);
}

} // namespace Krafter
