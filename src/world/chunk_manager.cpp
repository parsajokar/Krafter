#include "imgui.h"

#include "renderer/renderer.h"
#include "window.h"
#include "world/chunk_manager.h"

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
    glm::ivec2 chunkPosition = glm::ivec2(worldPosition.x, worldPosition.z) / Chunk::WIDTH;

    for (int32_t x = chunkPosition.x - _renderDistance; x <= chunkPosition.x + _renderDistance; x++) {
        for (int32_t z = chunkPosition.y - _renderDistance; z <= chunkPosition.y + _renderDistance; z++) {
            glm::ivec2 position = glm::ivec2(x, z);
            if (IsInRadius(position, chunkPosition, _renderDistance)) {
                if (!_chunkMap.count(position) && !_onChunkGenerationQueue.count(position)) {
                    _chunkGenerationQueue.push_back(position);
                    _onChunkGenerationQueue[position] = true;
                }
            }
        }
    }

    for (const auto& [position, chunk] : _chunkMap) {
        if (!IsInRadius(position, chunkPosition, _renderDistance) && !_onChunkDeletionQueue.count(position)) {
            _chunkDeletionQueue.push_back(position);
            _onChunkDeletionQueue[position] = true;
        }
    }

    if (!_chunkGenerationQueue.empty() && Window::Get()->GetTime() > _lastChunkGeneration + _chunkDelay) {
        _lastChunkGeneration = Window::Get()->GetTime();

        glm::ivec2 targetChunk = _chunkGenerationQueue.front();
        _chunkGenerationQueue.pop_front();
        _onChunkGenerationQueue.erase(targetChunk);

        if (IsInRadius(targetChunk, chunkPosition, _renderDistance)) {
            _chunkMap.emplace(targetChunk, std::move(Chunk(targetChunk)));
            Renderer::Get()->GenerateChunkMesh(_chunkMap, targetChunk);
        }
    }

    if (!_chunkDeletionQueue.empty() && Window::Get()->GetTime() > _lastChunkDeletion + _chunkDelay) {
        _lastChunkDeletion = Window::Get()->GetTime();

        glm::ivec2 targetChunk = _chunkDeletionQueue.front();
        _chunkDeletionQueue.pop_front();
        _onChunkDeletionQueue.erase(targetChunk);

        if (!IsInRadius(targetChunk, chunkPosition, _renderDistance)) {
            _chunkMap.erase(targetChunk);
            Renderer::Get()->DeleteChunkMesh(targetChunk);
        }
    }
}

void ChunkManager::OnRenderImGui()
{
    ImGui::InputInt("Render Distance", &_renderDistance);
    ImGui::InputFloat("Chunk Delay", &_chunkDelay);
}

} // namespace Krafter
